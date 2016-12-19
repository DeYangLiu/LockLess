#ifndef	_UTILS_H_
#define	_UTILS_H_

#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

#define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)

//min, max, rounding
#ifndef MIN
#define	MIN(x, y)	((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define	MAX(x, y)	((x) > (y) ? (x) : (y))
#endif

#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define	rounddown(x,y)	(((x)/(y))*(y))

#ifndef roundup2
#define	roundup2(x, m)	(((x) + (m) - 1) & ~((m) - 1))
#endif

// log2 on integer, fast division and remainder.
#ifndef flsl
static inline int
flsl(unsigned long x)
{
	return x ?
	    (sizeof(unsigned long) * CHAR_BIT) - __builtin_clzl(x) : 0;
}
#define	flsl(x)		flsl(x)
#endif
#ifndef flsll
static inline int
flsll(unsigned long long x)
{
	return x ?
	    (sizeof(unsigned long long) * CHAR_BIT) - __builtin_clzll(x) : 0;
}
#define	flsll(x)	flsl(x)
#endif

#ifndef ilog2
#define	ilog2(x)	(flsl(x) - 1)
#endif

//debug only
#if !defined(ASSERT)
#define	ASSERT	  assert
#endif

//compiler memory barrier
#define	CMM_barrier()	__asm__ __volatile__ ("" : : : "memory")
//disable prefetch and reordering
#define CMM_ACCESS_ONCE(x)	(*(__volatile__  __typeof__(x) *)&(x))
#define CMM_LOAD_SHARED(p)	({CMM_barrier(); CMM_ACCESS_ONCE(p);})
#define CMM_STORE_SHARED(x, v) ({ CMM_ACCESS_ONCE(x) = (v); CMM_barrier();})

//#include <stdatomic.h> //since GCC 4.9.0, this file exists.

//atomic oprations and memory barriers. name after C11 API
#ifndef atomic_thread_fence
#define	memory_order_relaxed    __atomic_thread_fence(__ATOMIC_RELAXED)
#define	memory_order_consume	__atomic_thread_fence(__ATOMIC_CONSUME)
#define	memory_order_acquire	__atomic_thread_fence(__ATOMIC_ACQUIRE)
#define	memory_order_release	__atomic_thread_fence(__ATOMIC_RELEASE)
#define	memory_order_acq_rel	__atomic_thread_fence(__ATOMIC_ACQ_REL)
#define	memory_order_seq_cst	__atomic_thread_fence(__ATOMIC_SEQ_CST)
#define	atomic_thread_fence(m)	m
#endif

#if defined(GCC_VERSION) && GCC_VERSION >= 40700
//since 4.7.0 we can use __atomic_xxx

#ifndef atomic_compare_exchange_strong
#define atomic_compare_exchange_strong(ptr, expected, desired) \
	__atomic_compare_exchange_n(ptr, expected, desired, false, \
									__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#endif

#ifndef atomic_compare_exchange_weak
#define atomic_compare_exchange_weak(ptr, expected, desired)  \
   __atomic_compare_exchange_n(ptr, expected, desired, true, \
									__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#endif


#ifndef atomic_compare_exchange_strong_explicit
#define atomic_compare_exchange_strong_explicit(ptr, expected, desired, mem_order_succ, mem_order_fail) \
	__atomic_compare_exchange_n(ptr, expected, desired, false, \
								mem_order_succ, mem_order_fail)
#endif

#ifndef atomic_compare_exchange_weak_explicit
#define atomic_compare_exchange_weak_explicit(ptr, expected, desired, mem_order_succ, mem_order_fail) \
	__atomic_compare_exchange_n(ptr, expected, desired, true,	\
								mem_order_succ, mem_order_fail)
#endif

#ifndef atomic_exchange
#define atomic_exchange(ptr, desired) \
	__atomic_exchange_n(ptr, desired, __ATOMIC_SEQ_CST)
#endif

#ifndef atomic_load
#define atomic_load(ptr) \
	__atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#endif

#ifndef atomic_store
#define atomic_store(ptr, desired) \
	__atomic_store_n(ptr, desired, __ATOMIC_SEQ_CST)
#endif

#elif defined(GCC_VERSION) && GCC_VERSION >= 40100
//since 4.1.0, we can use __sync_xxx
#ifndef atomic_compare_exchange_weak
 #define atomic_compare_exchange_weak(ptr, expected, desired) \
	 __sync_bool_compare_and_swap(ptr, *expected, desired)
#endif

#ifndef atomic_fetch_add
#define	atomic_fetch_add(x,a)	__sync_fetch_and_add(x, a)
#endif

#else
//platform-specific inline-assembly
#error "not support this GCC version"
#endif

#ifndef atomic_exchange
static inline void *
atomic_exchange(volatile void *ptr, void *nptr)
{
	volatile void * volatile old;

	do {
		old = (volatile void * volatile *)ptr;
	} while (!atomic_compare_exchange_weak(
	    (volatile void * volatile *)ptr, old, nptr));

	return (void *)(uintptr_t)old; // workaround for gcc warnings
}
#endif



//for spining
#define	SPINLOCK_BACKOFF_MIN	4
#define	SPINLOCK_BACKOFF_MAX	128
#if defined(__x86_64__) || defined(__i386__)
#define SPINLOCK_BACKOFF_HOOK	__asm volatile("pause" ::: "memory")
#else
#define SPINLOCK_BACKOFF_HOOK
#endif
#define	SPINLOCK_BACKOFF(count)					\
do {								\
	int __i;						\
	for (__i = (count); __i != 0; __i--) {			\
		SPINLOCK_BACKOFF_HOOK;				\
	}							\
	if ((count) < SPINLOCK_BACKOFF_MAX)			\
		(count) += (count);				\
} while (/* CONSTCOND */ 0);


#define	CACHE_LINE_SIZE		64


#endif
