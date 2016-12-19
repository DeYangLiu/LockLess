/*fast epoch base reclamation
*/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>


#include "gc.h"
#include "utils.h"

#include <stdio.h>
#define LOG(fmt, ...) \
	fprintf(stdout, "[%lx %s %d]"fmt, pthread_self(), __FUNCTION__, __LINE__,  ##__VA_ARGS__);

#define DBG(...) LOG(__VA_ARGS__)

#define EPOCH_SHIFT 2
#define EPOCH_MSK ((1<<EPOCH_SHIFT)-1)

#define EPOCHS 3

struct tls {
	unsigned count; //bit2: active, bit1,bit0: epoch
	struct tls *next;
};

struct gc {
    unsigned global_epoch;
	pthread_key_t tls_key;
	struct tls *list; //tls header
	gc_entry_t *garbage[EPOCHS];

	void (*reclaim)(gc_entry_t *);
	int mutex;
};

gc_t *gc_create(void (*reclaim)(gc_entry_t *))
{
	gc_t *g = calloc(1, sizeof *g);
	if(g){
		if(!pthread_key_create(&g->tls_key, NULL)){
			g->reclaim = reclaim;
		}else{
			free(g);
			g = NULL;
		}
	}

	return g;
}

void gc_destroy(gc_t *g)
{
	if(g){
		struct tls *t, *t2;
		for(t = g->list; t; t = t2){
			t2 = t->next;
			free(t);
		}
		pthread_key_delete(g->tls_key);
		free(g);
	}
}

static struct tls* tls_get(gc_t *g)
{
	struct tls *t, *head;

	t = pthread_getspecific(g->tls_key);
	if (!t) {
		if ((t = calloc(1, sizeof *t)) == NULL) {
			return NULL;
		}
		pthread_setspecific(g->tls_key, t);
		
		do {
			head = g->list;
			t->next = head;
		} while (!atomic_compare_exchange_weak(&g->list, &head, t));
	}
	return t;
}

void gc_free(gc_t *g, gc_entry_t *ent)
{
	gc_entry_t *head;
	struct tls *t;
	if(!g){
		return;
	}

	t = tls_get(g);
	
	if(t){
		unsigned e = t->count&EPOCH_MSK;
		do {
			head = g->garbage[e];
			ent->next = head;
		} while (!atomic_compare_exchange_weak(&g->garbage[e], &head, ent));
	}
}


static void reclaim(gc_t *g)
{
	unsigned e, tmp;
	struct tls *t;
	gc_entry_t *ent;
	unsigned expected = 0;
	
	if(!atomic_compare_exchange_strong(&g->mutex, &expected, 1)){
		return;
	}
	e = g->global_epoch;

	for(t = g->list; t; t = t->next){
		if(t == t->next){
			DBG("loop err\n");
			return;
		}

		{//do fast op
			//if not active, force to global epoch.
			expected = t->count&EPOCH_MSK;
			atomic_compare_exchange_strong(&t->count, &expected, e);
		}

		tmp = atomic_load(&t->count);
		//is this thread safe to free?
		// enter and epoch == e; -- yes
		// enter and epoch != e; -- no
		// exit and epoch == e -- yes
		// exit and epoch != e; -- impossible
		
		if((tmp&EPOCH_MSK) == e){
			
		}else{
			goto out;
		}
	}

	//now all threads see global epoch,
	//it's safe to free objects of two ages ago.
	ent	= atomic_exchange(&g->garbage[(e-2+EPOCHS)%EPOCHS], 0);
	if(ent){
		DBG("clean epoch %u\n", (e-2+EPOCHS)%EPOCHS);
		
		g->reclaim(ent);
	}

	//increase global epoch
	g->global_epoch = (e + 1)%EPOCHS;
	
out:
	g->mutex = 0;
	atomic_thread_fence(memory_order_release);
}


void gc_enter(gc_t *g)
{
	struct tls *t;
	unsigned e;

	if(!g){
		return;
	}
	t = tls_get(g);
	
	atomic_thread_fence(memory_order_acquire);
	e = g->global_epoch;
	
	t->count = (1<<EPOCH_SHIFT)|e;
	atomic_thread_fence(memory_order_release);
}

void gc_exit(gc_t *g)
{
	struct tls *t;
	if(!g){
		return;
	}
	t = tls_get(g);
	

	t->count = t->count & EPOCH_MSK;
	atomic_thread_fence(memory_order_release);

	reclaim(g);
}

void gc_full_flush(gc_t *g)
{
	unsigned i;
	if(!g){
		return;
	}
	
	for(i = 0; i < EPOCHS; ++i){
		if(g->garbage[i]){
			DBG("clean epoch %d\n", i);
			g->reclaim(g->garbage[i]);
			g->garbage[i] = NULL;
			
		}
	}
}

