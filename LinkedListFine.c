//fine-grained
#include <stdlib.h>
#include "utils.h"
#include "set.h"


struct Node{
	int key;
	char lock;
	struct Node *next;
};

static struct Node s_tail = {.key = INT_MAX};
static struct Node s_head = {.key = INT_MIN, .next = &s_tail};
static struct Node *head = &s_head;

#define lock(x) spin_lock(&x->lock)
#define release(x) spin_release(&x->lock)

static void spin_lock(char *lck)
{
	int cnt = SPINLOCK_BACKOFF_MIN;
	char expected;
again:
	atomic_thread_fence(memory_order_acquire);
	if(*lck){
		SPINLOCK_BACKOFF(cnt);
		
		if(cnt >= SPINLOCK_BACKOFF_MAX){
			//return;
		}
		goto again;
	}

	expected = 0;
	if(!atomic_compare_exchange_weak(lck, &expected, 1)){
		goto again;
	}
	
}

static void spin_release(char *lck)
{
	*lck = 0;
	atomic_thread_fence(memory_order_release);
}

static struct Node* newNode(int key)
{
	struct Node *n = malloc(sizeof *n);
	n->key = key;
	n->next = 0;
	n->lock = 0;
	return n;
}

static void freeNode(struct Node *n)
{
	if(n){
		free(n);
	}
}

int set_init(void)
{
	return 0;
}

void set_term(void)
{
}


int set_add(int key)
{
	bool ret =false;
	lock(head);
	struct Node *pred = head;
	struct Node *curr = pred->next;
	lock(curr);
	while(curr->key < key){
		release(pred);
		pred = curr;
		curr = curr->next;
		lock(curr);
	}
	if(curr->key != key){
		struct Node *n = newNode(key);
		n->next = curr;
		pred->next = n;
		ret = true;
	}

	release(curr);
	release(pred);
	return ret;
}

int set_remove(int key)
{
	bool ret = false;
	struct Node *pred, *curr;
	lock(head);
	pred = head;
	curr = pred->next;
	lock(curr);
	while(curr->key < key){
		release(pred);
		pred = curr;
		curr = curr->next;
		lock(curr);
	}
	if(curr->key == key){
		pred->next = curr->next;
		release(curr);
		freeNode(curr);
		ret = true;
	}else{
		release(curr);
	}

	release(pred);
	return ret;
}

int set_contains(int key)
{
	struct Node *pred, *curr;
	lock(head);
	pred = head;
	curr = pred->next;
	lock(curr);
	while(curr->key < key){
		release(pred);
		pred = curr;
		curr = curr->next;
		lock(curr);
	}
	bool ret = curr->key == key;
	release(curr);
	release(pred);
	return ret;
}
