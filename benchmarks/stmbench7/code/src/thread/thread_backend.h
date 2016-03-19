#ifndef THREAD_H
#define THREAD_H 1

#define ADAPT_TM 1

#include <pthread.h>
#include <stdlib.h>

#include "stm.h" // swisstm
#include "stm_tl2.h"
#include "stm_norec.h"
#include "stm_tinystm.h"
#include "mod_mem.h"
#include "mod_stats.h"
#include "wrappers.h"

# define CACHE_LINE_SIZE 64

 typedef struct tm_config {
       int backend;
       int num_threads;
       int htm_retry_policy;
       int htm_num_retries;
} tm_config_t;

typedef struct barrier {
    pthread_cond_t complete;
    pthread_mutex_t mutex;
    int count;
    int crossing;
} barrier_t;

typedef struct statistics {
    unsigned long commits;
    unsigned long aborts;
} __attribute__((aligned(CACHE_LINE_SIZE))) statistics_t;

typedef struct padded_statistics {
    statistics_t stats;
    char suffixPadding[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t;

typedef struct padded_scalar {
    volatile unsigned long counter;
    char suffixPadding[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t;

typedef struct padded_cond_var {
    pthread_cond_t cond_var;
    char suffixPadding[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) padded_cond_var_t;

typedef struct padded_mutex {
    pthread_mutex_t mutex;
    char suffixPadding[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) padded_mutex_t;

extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t statistics_array[];
extern __attribute__((aligned(CACHE_LINE_SIZE))) statistics_t last_statistics_array[];

extern int spawned_controller;
extern long total_threads;
extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t active_threads;
extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t htm_single_global_lock;
extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t fallback_in_use;
extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t exists_sw;

extern __thread unsigned short thread_id;
extern __thread int last_config_seen;
extern __thread int thread_state_before_barrier;
extern __thread norec_Thread* global_norec_thread;
extern __thread Thread* global_tl2_thread;
extern __thread unsigned short htm_tries;
extern __thread unsigned short read_only_htm;
extern __thread vwLock next_commit;

extern __attribute__((aligned(CACHE_LINE_SIZE))) padded_mutex_t reconfig_lock;
extern __attribute__((aligned(CACHE_LINE_SIZE))) barrier_t* reconfig_barrier;
extern barrier_t* reconfig_barrier_old;

extern padded_scalar_t* flag_is_running;
extern padded_cond_var_t* thread_cond_vars;
extern padded_mutex_t* thread_mutexes;

extern volatile int CURRENTMODE;
extern int DEFAULT_CONFIGURATION;
extern int DEFAULT_NUMBER_OF_THREADS;
extern int RECONFIGURING;

extern void greentm_handle_leaving_thread(int increment_val);
extern void greentm_thread_rejoin();
extern void greentm_thread_leave_to_block();
extern void greentm_before_tx();
extern void greentm_startup(int number_of_threads);
extern void greentm_shutdown();
extern void greentm_thread_enter();
extern void greentm_thread_exit();
extern void reconf_barrier_cross(barrier_t *b, int reset_count);
extern void adapt_threads(int target_number_threads);
extern void adapt_htm_retries(int new_htm_retry_policy, int new_htm_retry_budget);
extern void increment_threads(int target_number_threads);
extern void decrement_threads(int target_number_threads);
extern void reconfigure();

extern int tinystm_begin(int ro, sigjmp_buf* buf, int setjmp_ret);
extern void tinystm_end();
extern void tinystm_free(void* ptr);
extern void tinystm_abort();
extern intptr_t tinystm_sharedRead(intptr_t* addr);
extern void tinystm_sharedWrite(intptr_t* addr, intptr_t val);

extern int norec_begin(int ro, sigjmp_buf* buf, int setjmp_ret);
extern void norec_end();
extern void norec_free(void* ptr);
extern void norec_abort();
extern intptr_t norec_sharedRead(intptr_t* addr);
extern void norec_sharedWrite(intptr_t* addr, intptr_t val);

extern int tl2_begin(int ro, sigjmp_buf* buf, int setjmp_ret);
extern void tl2_end();
extern void tl2_free(void* ptr);
extern void tl2_abort();
extern intptr_t tl2_sharedRead(intptr_t* addr);
extern void tl2_sharedWrite(intptr_t* addr, intptr_t val);

extern int swisstm_begin(int ro, sigjmp_buf* buf, int setjmp_ret);
extern void swisstm_end();
extern void swisstm_free(void* ptr);
extern void swisstm_abort();
extern intptr_t swisstm_sharedRead(intptr_t* addr);
extern void swisstm_sharedWrite(intptr_t* addr, intptr_t val);

extern int htm_begin(int ro, sigjmp_buf* buf, int setjmp_ret);
extern void htm_end();
extern void htm_free(void* ptr);
extern void htm_abort();
extern intptr_t htm_sharedRead(intptr_t* addr);
extern void htm_sharedWrite(intptr_t* addr, intptr_t val);

extern int hybridNorec_begin(int ro, sigjmp_buf* buf, int setjmp_ret);
extern void hybridNorec_end();
extern void hybridNorec_htm_sharedWrite(intptr_t* addr, intptr_t val);

extern int hybridTl2_begin(int ro, sigjmp_buf* buf, int setjmp_ret);
extern void hybridTl2_end();
extern void hybridTl2_htm_sharedWrite(intptr_t* addr, intptr_t val);

extern void spend_budget(short* tries);

extern __thread void (*freeFunPtr)(void* ptr);
extern __thread void (*abortFunPtr)();
extern __thread intptr_t (*sharedReadFunPtr)(intptr_t* addr);
extern __thread void (*sharedWriteFunPtr)(intptr_t* addr, intptr_t val);
extern __thread int (*tmBeginFunPtr)(int ro, sigjmp_buf* buf, int setjmp_ret);
extern __thread void (*tmEndFunPtr)();


#ifdef __cplusplus
extern "C" {
#endif

#define NULL 0

#define THREAD_T                            pthread_t
#define THREAD_ATTR_T                       pthread_attr_t

#define THREAD_ATTR_INIT(attr)              pthread_attr_init(&attr)
#define THREAD_JOIN(tid)                    pthread_join(tid, (void**)NULL)
#define THREAD_CREATE(tid, attr, fn, arg)   pthread_create(&(tid), \
                                                           &(attr), \
                                                           (void* (*)(void*))(fn), \
                                                           (void*)(arg))

#define THREAD_LOCAL_T                      pthread_key_t
#define THREAD_LOCAL_INIT(key)              pthread_key_create(&key, NULL)
#define THREAD_LOCAL_SET(key, val)          pthread_setspecific(key, (void*)(val))
#define THREAD_LOCAL_GET(key)               pthread_getspecific(key)

#define THREAD_MUTEX_T                      pthread_mutex_t
#define THREAD_MUTEX_INIT(lock)             pthread_spin_init(&(lock), NULL)
#define THREAD_MUTEX_LOCK(lock)             pthread_mutex_lock(&(lock))
#define THREAD_MUTEX_UNLOCK(lock)           pthread_mutex_unlock(&(lock))

#define THREAD_COND_T                       pthread_cond_t
#define THREAD_COND_INIT(cond)              pthread_cond_init(&(cond), NULL)
#define THREAD_COND_SIGNAL(cond)            pthread_cond_signal(&(cond))
#define THREAD_COND_BROADCAST(cond)         pthread_cond_broadcast(&(cond))
#define THREAD_COND_WAIT(cond, lock)        pthread_cond_wait(&(cond), &(lock))

#  define THREAD_BARRIER_T                  barrier_t
#  define THREAD_BARRIER_ALLOC(N)           barrier_alloc()
#  define THREAD_BARRIER_INIT(bar, N)       barrier_init(bar, N)
#  define THREAD_BARRIER(bar, tid)          barrier_cross(bar,1)
#  define THREAD_BARRIER_FREE(bar)          barrier_free(bar)


barrier_t *barrier_alloc();

void barrier_free(barrier_t *b);

void barrier_init(barrier_t *b, int n);

void barrier_cross(barrier_t *b, int reset_count);

void thread_startup (long numThread);

void thread_start (void (*funcPtr)(void*), void* argPtr);

void thread_shutdown ();

void thread_barrier_wait();

long thread_getId();

long thread_getNumThread();


//

#ifdef __cplusplus
}
#endif

#endif /* THREAD_H */
