#define _GNU_SOURCE
#include <stdint.h>
#include <alloca.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <setjmp.h>
#ifdef __SSE__
# include <xmmintrin.h>
#endif /* __SSE__ */

#define REDUCED_TM_API 1
#define GCC_RECTM 1

#include "libitm.h"
#include "../thread.h"
#include "../thread.c"
#include "../tm.h"

#ifdef __PIC__
/* Add call when the library is loaded and unloaded */
#define ATTR_CONSTRUCTOR __attribute__ ((constructor))
#define ATTR_DESTRUCTOR __attribute__ ((destructor))
#else
#define ATTR_CONSTRUCTOR
#define ATTR_DESTRUCTOR
#endif

#define INLINE inline __attribute__((always_inline))
#ifndef NULL
  #define NULL 0
#endif

/* Indicates to use _ITM_siglongjmp */
# define CTX_ITM   _ITM_siglongjmp
extern void _ITM_CALL_CONVENTION _ITM_siglongjmp(int val, sigjmp_buf env) __attribute__ ((noreturn));

/* ################################################################### *
 * VARIABLES
 * ################################################################### */
/* Status of the ABI */
enum {
    ABI_NOT_INITIALIZED,
    ABI_INITIALIZED,
    ABI_FINALIZED,
};

static union {
    struct {
        volatile unsigned long status;
        volatile unsigned long thread_counter;
    };
    uint8_t padding[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) global_abi = {{.status = ABI_NOT_INITIALIZED, .thread_counter = 0 }};

pthread_mutex_t startup_lock = PTHREAD_MUTEX_INITIALIZER;

// __attribute__((aligned(CACHE_LINE_SIZE))) __thread greentm_tx_t * thread_tx;

/* ################################################################### *
 * COMPATIBILITY FUNCTIONS
 * ################################################################### */
# include <malloc.h>
INLINE size_t block_size(void *ptr) {
    return malloc_usable_size(ptr);
}

static INLINE void tls_init(void) {
}

static INLINE void tls_exit(void) {
}

static INLINE int get_tid() {
    return thread_id;
}

static INLINE void abi_init_thread(void) {
    statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level = 0;
    statistics_array[SPECIAL_THREAD_ID()].stats.has_restarted = 0;
    if (global_abi.status == ABI_NOT_INITIALIZED) {
        _ITM_initializeProcess();
    }
    thread_id = __sync_fetch_and_add(&global_abi.thread_counter, 1);
}

extern void reset_nesting_level();

/* ################################################################### *
 * FUNCTIONS
 * ################################################################### */

int _ITM_CALL_CONVENTION _ITM_initializeThread(void)
{
    abi_init_thread();
    TM_THREAD_ENTER();
    return 0;
}

int _ITM_CALL_CONVENTION _ITM_initializeUnblockableThread(void)
{
    abi_init_thread();
    TM_THREAD_ENTER_UNBLOCKABLE();
    return 0;
}

void _ITM_CALL_CONVENTION _ITM_finalizeThread(void) {
    TM_THREAD_EXIT();
}

int ATTR_CONSTRUCTOR _ITM_CALL_CONVENTION _ITM_initializeProcess(void)
{
    pthread_mutex_lock(&startup_lock);
    if (global_abi.status == ABI_NOT_INITIALIZED) {
        global_abi.status = ABI_INITIALIZED;
        printf("\n\t====\tInitializing GreenTM\t====\t\n");
        atexit((void (*)(void))(_ITM_finalizeProcess));
        TM_STARTUP(8,0); // Note: this number of threads is arbitrary (i.e., only needs to be > 0)
    }
    pthread_mutex_unlock(&startup_lock);
    return 0;
}

void ATTR_DESTRUCTOR _ITM_CALL_CONVENTION _ITM_finalizeProcess(void)
{
    pthread_mutex_lock(&startup_lock);
    if (global_abi.status == ABI_INITIALIZED) {
        global_abi.status = ABI_FINALIZED;
        printf("\n\t====\tShutting down GreenTM\t====\t\n");
        TM_SHUTDOWN();
    }
    pthread_mutex_unlock(&startup_lock);
}

void reset_nesting_level() {
    statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level = 1;
    statistics_array[SPECIAL_THREAD_ID()].stats.aborts++;
}


/* The _ITM_beginTransaction is defined in assembly (arch.S)  */
uint32_t _ITM_CALL_CONVENTION GTM_begin_transaction(uint32_t attr, jmp_buf * buf) {
    uint32_t ret;

    int ro = attr & pr_readOnly;
    unsigned short have_restarted = statistics_array[SPECIAL_THREAD_ID()].stats.has_restarted;
    if (have_restarted == 0) {
        ret |= a_saveLiveVariables;
    } else if (have_restarted > 0) {
        ret |= a_restoreLiveVariables;
    }

    unsigned short nesting_level = ++(statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level);
    if (nesting_level == 1) {
        // Save thread context only when outermost transaction
        greentm_before_tx();
        jmp_buf* stm_buf = wlpdstm_get_long_jmp_buf();
        if ((*tmBeginFunPtr)(ro, stm_buf, 0)) {
            ret |= a_runInstrumentedCode;
            memcpy(stm_buf, buf, sizeof(jmp_buf));
        } else {
            ret |= a_runUninstrumentedCode;
        }
        statistics_array[SPECIAL_THREAD_ID()].stats.aborts++;
        statistics_array[SPECIAL_THREAD_ID()].stats.has_restarted = 1;
    }

    return ret;
}

void _ITM_CALL_CONVENTION _ITM_commitTransaction(void) {
    if ((--statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level) == 0) {
        (*tmEndFunPtr)();
        NO_LONGER_EXEC_TXS(thread_id);
        statistics_array[SPECIAL_THREAD_ID()].stats.aborts--;
        statistics_array[SPECIAL_THREAD_ID()].stats.commits++;
        statistics_array[SPECIAL_THREAD_ID()].stats.has_restarted = 0;
    }
}

bool _ITM_CALL_CONVENTION _ITM_tryCommitTransaction(const _ITM_srcLocation *__src) {
    _ITM_commitTransaction();
    return 1;
}

void _ITM_CALL_CONVENTION _ITM_abortTransaction(_ITM_abortReason __reason, const _ITM_srcLocation *__src) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
    _ITM_rollbackTransaction(__src);
}

void _ITM_CALL_CONVENTION _ITM_rollbackTransaction(const _ITM_srcLocation *__src) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
    statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level = 0;
    FAST_PATH_RESTART();
}

_ITM_transaction* _ITM_CALL_CONVENTION _ITM_getTransaction(void) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);

    return (_ITM_transaction *)&statistics_array[SPECIAL_THREAD_ID()].stats;
}

_ITM_howExecuting _ITM_CALL_CONVENTION _ITM_inTransaction() {
    if (statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level > 0) {
        return inRetryableTransaction;
    }
    return outsideTransaction;
}

int _ITM_CALL_CONVENTION _ITM_getThreadnum(void)
{
    return get_tid();
}

void * _ITM_malloc(size_t size) {
    return malloc(size);
}

void * _ITM_calloc(size_t nm, size_t size) {
    return calloc(nm, size);
}

void _ITM_free(void *ptr) {
    SLOW_PATH_FREE(ptr);
}

void _ITM_CALL_CONVENTION _ITM_error(const _ITM_srcLocation *__src, int errorCode)
{
    fprintf(stderr, "Error: %s (%d)\n", (__src == NULL || __src->psource == NULL ? "?" : __src->psource), errorCode);
    exit(1);
}

void _ITM_CALL_CONVENTION _ITM_userError(const char *errString, int exitCode) {
    fprintf(stderr, "%s", errString);
    exit(exitCode);
}

const char * _ITM_CALL_CONVENTION _ITM_libraryVersion(void) {
    return _ITM_VERSION_NO_STR " using GreenTM 0.1";
}

int _ITM_CALL_CONVENTION _ITM_versionCompatible(int version) {
    return version == _ITM_VERSION_NO;
}

/* ################################################################### *
 * Not yet implemented: we should not need these functions
 * ################################################################### */

void _ITM_CALL_CONVENTION _ITM_registerThrownObject(const void *__obj, size_t __size) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
}

void _ITM_CALL_CONVENTION _ITM_changeTransactionMode(_ITM_transactionState __mode, const _ITM_srcLocation *__loc) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
}

void _ITM_CALL_CONVENTION _ITM_commitTransactionEH(void *exc_ptr) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
}

void _ITM_CALL_CONVENTION _ITM_commitTransactionToId(const _ITM_transactionId tid, const _ITM_srcLocation *__src) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
}

void _ITM_CALL_CONVENTION _ITM_dropReferences(const void *__start, size_t __size) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
}

void _ITM_CALL_CONVENTION _ITM_addUserCommitAction(_ITM_userCommitFunction __commit, _ITM_transactionId resumingTransactionId, void *__arg) {
    // TODO this is being called in memcached, but it is not yet implemented
    // fprintf(stderr, "%s: not yet implemented\n", __func__);
    // exit(1);
}

void _ITM_CALL_CONVENTION _ITM_addUserUndoAction(const _ITM_userUndoFunction __undo, void * __arg) {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
}

_ITM_transactionId _ITM_CALL_CONVENTION _ITM_getTransactionId() {
    fprintf(stderr, "%s: not yet implemented\n", __func__);
    exit(1);
}


# include "clone.c"
# include "eh.c"

/**** LOAD STORE LOG FUNCTIONS ****/

#define TM_LOAD(F, T, WF, WT, M) \
  T _ITM_CALL_CONVENTION F(const T *addr) \
  { \
    return (*sharedRead##M##FunPtr)(addr); \
  }

#define TM_LOAD_GENERIC(F, T) \
  T _ITM_CALL_CONVENTION F(const T *addr) \
  { \
    fprintf(stderr, "%s: not yet implemented\n", __func__); \
    exit(1); \
  }

#define TM_STORE(F, T, WF, WT, M) \
  void _ITM_CALL_CONVENTION F(const T *addr, T val) \
  { \
    (*sharedWrite##M##FunPtr)(addr, val); \
  }

#define TM_STORE_GENERIC(F, T) \
  void _ITM_CALL_CONVENTION F(const T *addr, T val) \
  { \
    fprintf(stderr, "%s: not yet implemented\n", __func__); \
    exit(1); \
  }

#define TM_LOG(F, T, WF, WT, M) \
  void _ITM_CALL_CONVENTION F(const T *addr) \
  { \
	(*sharedRead##M##FunPtr)(addr); \
  }

#define TM_LOG_GENERIC(F, T) \
  void _ITM_CALL_CONVENTION F(const T *addr) \
  { \
    fprintf(stderr, "%s: not yet implemented\n", __func__); \
    exit(1); \
  }

#define TM_STORE_BYTES(F) \
  void _ITM_CALL_CONVENTION F(void *dst, const void *src, size_t size) \
  { \
    fprintf(stderr, "%s: not yet implemented\n", __func__); \
    exit(1); \
  }

#define TM_LOAD_BYTES(F) \
  void _ITM_CALL_CONVENTION F(void *dst, const void *src, size_t size) \
  { \
    fprintf(stderr, "%s: not yet implemented\n", __func__); \
    exit(1); \
  }

#define TM_LOG_BYTES(F) \
  void _ITM_CALL_CONVENTION F(const void *addr, size_t size) \
  { \
    fprintf(stderr, "%s: not yet implemented\n", __func__); \
    exit(1); \
  }

#define TM_SET_BYTES(F) \
  void _ITM_CALL_CONVENTION F(void *dst, int val, size_t count) \
  { \
    fprintf(stderr, "%s: not yet implemented\n", __func__); \
    exit(1); \
  }

#define TM_COPY_BYTES(F) \
  void _ITM_CALL_CONVENTION F(void *dst, const void *src, size_t size) \
  { \
        /*int i = 0;*/ \
        /*for (; i < size; i++) {*/ \
                /*(*sharedWriteU8FunPtr)(&(((uint8_t*)dst)[i]), (*sharedReadU8FunPtr)(&((const uint8_t*)src)[i])); */\
        /*}*/ \
        memcpy(dst, src, size); \
  }

#define TM_COPY_BYTES_RN_WT(F) \
  void _ITM_CALL_CONVENTION F(void *dst, const void *src, size_t size) \
  { \
      memcpy(dst, src, size); \
  }

#define TM_COPY_BYTES_RT_WN(F) \
  void _ITM_CALL_CONVENTION F(void *dst, const void *src, size_t size) \
  { \
    fprintf(stderr, "%s: not yet implemented\n", __func__); \
    exit(1); \
  }

#define TM_LOAD_ALL(E, T, WF, WT, M) \
  TM_LOAD(_ITM_R##E, T, WF, WT, M) \
  TM_LOAD(_ITM_RaR##E, T, WF, WT, M) \
  TM_LOAD(_ITM_RaW##E, T, WF, WT, M) \
  TM_LOAD(_ITM_RfW##E, T, WF, WT, M)

#define TM_LOAD_GENERIC_ALL(E, T) \
  TM_LOAD_GENERIC(_ITM_R##E, T) \
  TM_LOAD_GENERIC(_ITM_RaR##E, T) \
  TM_LOAD_GENERIC(_ITM_RaW##E, T) \
  TM_LOAD_GENERIC(_ITM_RfW##E, T)

#define TM_STORE_ALL(E, T, WF, WT, M) \
  TM_STORE(_ITM_W##E, T, WF, WT, M) \
  TM_STORE(_ITM_WaR##E, T, WF, WT, M) \
  TM_STORE(_ITM_WaW##E, T, WF, WT, M)

#define TM_STORE_GENERIC_ALL(E, T) \
  TM_STORE_GENERIC(_ITM_W##E, T) \
  TM_STORE_GENERIC(_ITM_WaR##E, T) \
  TM_STORE_GENERIC(_ITM_WaW##E, T)


TM_LOAD_ALL(U1, uint8_t, int_stm_load_u8, uint8_t, U8)
TM_LOAD_ALL(U2, uint16_t, int_stm_load_u16, uint16_t, U16)
TM_LOAD_ALL(U4, uint32_t, int_stm_load_u32, uint32_t, U32)
TM_LOAD_ALL(U8, uint64_t, int_stm_load_u64, uint64_t, U64)
TM_LOAD_ALL(F, float, stm_load_float, float, U64)
TM_LOAD_ALL(D, double, stm_load_double, double, U64)
TM_LOAD_GENERIC_ALL(CF, float _Complex)
TM_LOAD_GENERIC_ALL(CD, double _Complex)
TM_LOAD_GENERIC_ALL(CE, long double _Complex)

TM_STORE_ALL(U1, uint8_t, int_stm_store_u8, uint8_t, U8)
TM_STORE_ALL(U2, uint16_t, int_stm_store_u16, uint16_t, U16)
TM_STORE_ALL(U4, uint32_t, int_stm_store_u32, uint32_t, U32)
TM_STORE_ALL(U8, uint64_t, int_stm_store_u64, uint64_t, U64)
TM_STORE_ALL(F, float, stm_store_float, float, U64)
TM_STORE_ALL(D, double, stm_store_double, double, U64)
TM_STORE_GENERIC_ALL(CF, float _Complex)
TM_STORE_GENERIC_ALL(CD, double _Complex)
TM_STORE_GENERIC_ALL(CE, long double _Complex)

TM_STORE_BYTES(_ITM_memcpyRnWt)
TM_STORE_BYTES(_ITM_memcpyRnWtaR)
TM_STORE_BYTES(_ITM_memcpyRnWtaW)

TM_LOAD_BYTES(_ITM_memcpyRtWn)
TM_LOAD_BYTES(_ITM_memcpyRtaRWn)
TM_LOAD_BYTES(_ITM_memcpyRtaWWn)

TM_COPY_BYTES(_ITM_memcpyRtWt)
TM_COPY_BYTES(_ITM_memcpyRtWtaR)
TM_COPY_BYTES(_ITM_memcpyRtWtaW)
TM_COPY_BYTES(_ITM_memcpyRtaRWt)
TM_COPY_BYTES(_ITM_memcpyRtaRWtaR)
TM_COPY_BYTES(_ITM_memcpyRtaRWtaW)
TM_COPY_BYTES(_ITM_memcpyRtaWWt)
TM_COPY_BYTES(_ITM_memcpyRtaWWtaR)
TM_COPY_BYTES(_ITM_memcpyRtaWWtaW)

TM_LOG(_ITM_LU1, uint8_t, stm_log_u8, uint8_t, U8)
TM_LOG(_ITM_LU2, uint16_t, stm_log_u16, uint16_t, U16)
TM_LOG(_ITM_LU4, uint32_t, stm_log_u32, uint32_t, U32)
TM_LOG(_ITM_LU8, uint64_t, stm_log_u64, uint64_t, U64)
TM_LOG(_ITM_LF, float, stm_log_float, float, U64)
TM_LOG(_ITM_LD, double, stm_log_double, double, U64)
TM_LOG_GENERIC(_ITM_LE, long double)
TM_LOG_GENERIC(_ITM_LCF, float _Complex)
TM_LOG_GENERIC(_ITM_LCD, double _Complex)
TM_LOG_GENERIC(_ITM_LCE, long double _Complex)

TM_LOG_BYTES(_ITM_LB)

TM_SET_BYTES(_ITM_memsetW)
TM_SET_BYTES(_ITM_memsetWaR)
TM_SET_BYTES(_ITM_memsetWaW)

TM_COPY_BYTES_RN_WT(_ITM_memmoveRnWt)
TM_COPY_BYTES_RN_WT(_ITM_memmoveRnWtaR)
TM_COPY_BYTES_RN_WT(_ITM_memmoveRnWtaW)

TM_COPY_BYTES_RT_WN(_ITM_memmoveRtWn)
TM_COPY_BYTES_RT_WN(_ITM_memmoveRtaRWn)
TM_COPY_BYTES_RT_WN(_ITM_memmoveRtaWWn)

TM_COPY_BYTES(_ITM_memmoveRtWt)
TM_COPY_BYTES(_ITM_memmoveRtWtaR)
TM_COPY_BYTES(_ITM_memmoveRtWtaW)
TM_COPY_BYTES(_ITM_memmoveRtaRWt)
TM_COPY_BYTES(_ITM_memmoveRtaRWtaR)
TM_COPY_BYTES(_ITM_memmoveRtaRWtaW)
TM_COPY_BYTES(_ITM_memmoveRtaWWt)
TM_COPY_BYTES(_ITM_memmoveRtaWWtaR)
TM_COPY_BYTES(_ITM_memmoveRtaWWtaW)
