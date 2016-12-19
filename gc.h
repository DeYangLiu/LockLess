
#ifndef _GC_H_
#define _GC_H_

typedef struct gc gc_t;

typedef struct gc_entry {
	struct gc_entry *next;
} gc_entry_t;


gc_t *gc_create(void (*)(gc_entry_t *));
void gc_full_flush(gc_t *g);
void gc_destroy(gc_t *g);

void gc_enter(gc_t *g);
void gc_free(gc_t *g, gc_entry_t *);
void gc_exit(gc_t *g);

#endif
