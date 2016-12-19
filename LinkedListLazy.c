//lazy: mark and delay free
#include <stdlib.h>
#include <string.h>
#include <stddef.h> //for offsetof
#include <stdio.h>
#include <pthread.h>

#include "utils.h"
#include "gc.h"
#include "set.h"


#ifdef DEBUG
#define DBG(...) LOG(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define LOG(fmt, ...) \
	fprintf(stdout, "[%lx %s %d]"fmt, pthread_self(), __FUNCTION__, __LINE__,  ##__VA_ARGS__); 


struct Node{
	int key;
	char marked;
	int lock;
	//pthread_mutex_t lock;
	gc_entry_t gc_entry;
	struct Node *next;
};

static struct Node s_tail = {.key = INT_MAX};
static struct Node s_head = {.key = INT_MIN, .next = &s_tail};
static struct Node *head = &s_head;

#define LOCK(prev, curr) do{ \
	_spin_lock(prev); \
	if(prev != curr){ \
		_spin_lock(curr); \
	} \
}while(0)

#define RELS(prev, curr) do{ \
	_spin_release(curr); \
	if(prev != curr){ \
		_spin_release(prev); \
	} \
}while(0)

static gc_t *s_gc = NULL;

static void _spin_lock(struct Node *n)
{
	#if 0
	return pthread_mutex_lock(&n->lock);
	#else

	#ifdef DEBUG
	int err = 0;
	#endif
	
	int cnt = SPINLOCK_BACKOFF_MIN;
	int expected;
again:
	atomic_thread_fence(memory_order_acquire);
	if(n->lock){
		SPINLOCK_BACKOFF(cnt);
		
		if(cnt >= SPINLOCK_BACKOFF_MAX){
			#ifdef DEBUG
			if(!(err%10000)){
				int next_key = n->next ? n->next->key : -2;
				DBG("%p key %d %d\n", n, n->key, next_key);
			}
			++err;
			#endif
		}
		goto again;
	}

	expected = 0;
	if(!atomic_compare_exchange_weak(&n->lock, &expected, 1)){
		goto again;
	}
	
	#endif
}

static void _spin_release(struct Node *n)
{
	#if 0
	pthread_mutex_unlock(&n->lock);
	#else
	
	n->lock = 0;
	atomic_thread_fence(memory_order_release);
	#endif
	DBG("%p key %d\n", n, n->key);
}


static struct Node* newNode(int key)
{
	struct Node *n = malloc(sizeof *n);
	memset(n, 0, sizeof *n);
	n->key = key;
	//pthread_mutex_init(&n->lock, NULL);
	return n;
}

static void freeNode(struct Node *n)
{
	if(n){
		//logical delete
		//don't change the fields which other thread is visiting
		gc_free(s_gc, &n->gc_entry);
	}
}

static void phy_free(gc_entry_t *ent)
{
	for(; ent; ){
		struct Node *obj = (struct Node *)((uintptr_t)ent - 
			offsetof(struct Node, gc_entry));
		ent = ent->next;

		DBG("obj %p key %d\n", obj, obj->key);
		free(obj);
	}
}

static bool validate(struct Node *pred, struct Node *curr)
{
	return !pred->marked && !curr->marked && pred->next == curr;
}


int set_init(void)
{
	if(!s_gc){
		gc_t *gc = gc_create(phy_free);
		(void)atomic_exchange(&s_gc, gc);
	}

	if(!s_gc){
		return -1;
	}
	return 0;
}

void set_term(void)
{
	gc_t *gc = atomic_exchange(&s_gc, NULL);
	if(gc){
		LOG("fini\n");
		gc_full_flush(gc);
		gc_destroy(gc);
	}
}


int set_add(int key)
{
	bool ret;
	gc_enter(s_gc);
retry:
	
	ret = false;
	struct Node *pred = head;
	struct Node *curr = pred->next;
	while(curr->key < key){
		pred = curr; curr = curr->next;
	}

	LOCK(pred, curr);
	if(validate(pred, curr)){
		if(curr->key == key){
			ret = false;
		}else{
			struct Node *node = newNode(key);
			node->next = curr;
			pred->next = node;
			ret = true;
			DBG("%p key %d\n", node, key);
		}
	}else{
		RELS(pred, curr);
		//DBG("%p %p retry key %d\n", pred, curr, key);
		goto retry;
	}
	RELS(pred, curr);

	gc_exit(s_gc);
	return ret;
}

int set_remove(int key)
{
	bool ret;
	DBG("to key %d\n", key);
	int err = 0;
	gc_enter(s_gc);
retry:

	ret = false;
	struct Node *pred = head;
	struct Node *curr = pred->next;
	while(curr->key < key){
		if(curr == curr->next){
			LOG("loop err %d %d\n", pred->key, curr->key);
			return 0;
		}
		pred = curr; curr = curr->next;
	}

	LOCK(pred, curr);
	if(validate(pred, curr)){
		if(curr->key != key){
			ret = false;
		}else{
			curr->marked = true;
			pred->next = curr->next;
			ret = true;
			DBG("%p key %d\n", curr, key);
			
			freeNode(curr);
		}
	}else{
		RELS(pred, curr);
		
		DBG("%p %p retry key %d err %d\n", pred, curr, key, err);
		++err;
		goto retry;
	}
	RELS(pred, curr);
	gc_exit(s_gc);

	DBG("end key %d\n", key);
	return ret;
}

int set_contains(int key)
{
	struct Node *curr = head;
	
	gc_enter(s_gc);
	while(curr->key < key){
		curr = curr->next;
	}
	int ret = curr->key == key && !curr->marked;
	gc_exit(s_gc);
	
	return ret;
}

