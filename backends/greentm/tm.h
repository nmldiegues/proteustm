#ifndef TM_H
#define TM_H 1

#  include <stdio.h>

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    /* nothing */
#  define GOTO_REAL()                   /* nothing */
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */

# define AL_LOCK(b)
# define PRINT_STATS()
# define SETUP_NUMBER_TASKS(b)
# define SETUP_NUMBER_THREADS(b)

#  include <assert.h>
#ifndef REDUCED_TM_API
#  include "memory.h"
#  include "thread.h"
#  include "types.h"
#endif

#include <immintrin.h>
#include <rtmintrin.h>

# ifdef REDUCED_TM_API
#    define SPECIAL_THREAD_ID()         get_tid()
#else
#    define SPECIAL_THREAD_ID()         thread_getId()
#endif

#    define TM_ARG_ALONE
#    define TM_ARGDECL
#    define TM_ARGDECL_ALONE
#    define TM_CALLABLE
#    define TM_ARG

#    define TM_THREAD_ENTER()  \
                thread_id = SPECIAL_THREAD_ID(); \
                greentm_thread_enter(1); \

#    define TM_THREAD_ENTER_UNBLOCKABLE() \
                thread_id = SPECIAL_THREAD_ID(); \
                greentm_thread_enter(0); \

#    define TM_THREAD_EXIT()  greentm_thread_exit()

#    define TM_STARTUP(numThread,u)     greentm_startup(numThread)
#    define TM_SHUTDOWN()             greentm_shutdown()

#  define TM_BEGIN_WAIVER()
#  define TM_END_WAIVER()

# define TM_BEGIN(b,mode)     TM_BEGIN_EXT(b,mode,0)

# define TM_BEGIN_EXT(b,mode,ro) \
    { \
        unsigned short priv_nesting_level = ++(statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level); \
        if (priv_nesting_level == 1) { \
            greentm_before_tx(); \
            sigjmp_buf stm_buf; \
            int setjmp_ret = sigsetjmp(stm_buf, 0); \
            mode = (*tmBeginFunPtr)(ro, &stm_buf, setjmp_ret); \
            statistics_array[SPECIAL_THREAD_ID()].stats.aborts++; \
        } \

#    define TM_END() \
        if ((--statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level) == 0) { \
            (*tmEndFunPtr)(); \
            NO_LONGER_EXEC_TXS(thread_id); \
            statistics_array[SPECIAL_THREAD_ID()].stats.aborts--; \
            statistics_array[SPECIAL_THREAD_ID()].stats.commits++; \
        } \
    } \

#    define TM_EARLY_RELEASE(var)         


#      define P_MALLOC(size)            malloc(size)
#      define P_FREE(ptr)               free(ptr)
#      define SEQ_MALLOC(size)          malloc(size)
#      define SEQ_FREE(ptr)             free(ptr)

#      define TM_MALLOC(size)           malloc(size)
#      define FAST_PATH_FREE(ptr)        free(ptr)
#      define SLOW_PATH_FREE(ptr)        (*freeFunPtr)((void*)ptr)


# define SLOW_PATH_RESTART() statistics_array[SPECIAL_THREAD_ID()].stats.nesting_level = 1; (*abortFunPtr)();
# define SLOW_PATH_SHARED_READ(var)  (*sharedReadFunPtr)((vintp*)(void*)&(var))
# define SLOW_PATH_SHARED_READ_P(var) norec_IP2VP(SLOW_PATH_SHARED_READ(var))
# define SLOW_PATH_SHARED_READ_D(var) norec_IP2D((*sharedReadFunPtr)((vintp*)norec_DP2IPP(&(var))))
# define SLOW_PATH_SHARED_WRITE(var, val) (*sharedWriteFunPtr)((vintp*)(void*)&(var), (intptr_t)val)
# define SLOW_PATH_SHARED_WRITE_P(var, val) (*sharedWriteFunPtr)((vintp*)(void*)&(var), norec_VP2IP(val))
# define SLOW_PATH_SHARED_WRITE_D(var, val) (*sharedWriteFunPtr)((vintp*)norec_DP2IPP(&(var)), norec_D2IP(val))

# define FAST_PATH_RESTART()                  SLOW_PATH_RESTART()
# define FAST_PATH_SHARED_READ(var)           (var)
# define FAST_PATH_SHARED_READ_P(var)         (var)
# define FAST_PATH_SHARED_READ_F(var)         (var)
# define FAST_PATH_SHARED_READ_D(var)         (var)
# define FAST_PATH_SHARED_WRITE(var, val)     ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_P(var, val)   ({var = val; var;})
# define FAST_PATH_SHARED_WRITE_D(var, val)   ({var = val; var;})

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})

#endif /* TM_H */
