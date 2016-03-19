/* =============================================================================
 *
 * stm.h
 *
 * User program interface for STM. For an STM to interface with STAMP, it needs
 * to have its own stm.h for which it redefines the macros appropriately.
 *
 * =============================================================================
 *
 * Author: Chi Cao Minh
 *
 * =============================================================================
 */


#ifndef norec_STM_H
#define norec_STM_H 1


#include "norec.h"
#include "norec_util.h"

#define norec_STM_THREAD_T                    norec_Thread
#define norec_STM_SELF                        norec_Self
#define norec_STM_RO_FLAG                     norec_ROFlag

#  include <setjmp.h>
#  define norec_STM_JMPBUF_T                  sigjmp_buf
#  define norec_STM_JMPBUF                    norec_buf


#define norec_STM_VALID()                     (1)
#define norec_STM_RESTART()                   norec_TxAbort(norec_STM_SELF)

#define norec_STM_STARTUP()                   norec_TxOnce()
#define norec_STM_SHUTDOWN()                  norec_TxShutdown()

#define norec_STM_NEW_THREAD()                norec_TxNewThread()
#define norec_STM_INIT_THREAD(t, id)          norec_TxInitThread(t, id)
#define norec_STM_FREE_THREAD(t)              norec_TxFreeThread(t)








#  define norec_STM_BEGIN(isReadOnly)         do { \
                                            norec_STM_JMPBUF_T norec_STM_JMPBUF; \
                                            sigsetjmp(norec_STM_JMPBUF, 0); \
                                            norec_TxStart(norec_STM_SELF, &norec_STM_JMPBUF); \
                                        } while (0) /* enforce comma */

#define norec_STM_BEGIN_RD()                  norec_STM_BEGIN(1)
#define norec_STM_BEGIN_WR()                  norec_STM_BEGIN(0)
#define norec_TX_END_HYBRID_FIRST_STEP()      norec_TxValidate(norec_STM_SELF)
#define norec_TX_END_HYBRID_LAST_STEP(clock)  norec_TxFinalize(norec_STM_SELF, clock)
#define norec_TX_AFTER_FINALIZE()             norec_TxResetAfterFinalize (norec_STM_SELF)
#define norec_HYBRID_STM_END()                norec_TxCommitSTM(norec_STM_SELF)
#define norec_STM_END()                       norec_TxCommit(norec_STM_SELF)

typedef volatile intptr_t               vintp;

#define norec_STM_HYBRID_READ(varPtr)         norec_TxLoad(norec_STM_SELF, varPtr)

#define norec_STM_READ(var)                   norec_TxLoad(norec_STM_SELF, (vintp*)(void*)&(var))
#define norec_STM_READ_D(var)                 norec_IP2D(norec_TxLoad(norec_STM_SELF, \
                                                    (vintp*)norec_DP2IPP(&(var))))
#define norec_STM_READ_P(var)                 norec_IP2VP(norec_TxLoad(norec_STM_SELF, \
                                                     (vintp*)(void*)&(var)))

#define norec_STM_HYBRID_WRITE(varPtr, val)   norec_TxStore(norec_STM_SELF, varPtr, val)
#define norec_STM_WRITE(var, val)             norec_TxStore(norec_STM_SELF, \
                                                (vintp*)(void*)&(var), \
                                                (intptr_t)(val))
#define norec_STM_WRITE_D(var, val)           norec_TxStore(norec_STM_SELF, \
                                                (vintp*)norec_DP2IPP(&(var)), \
                                                norec_D2IP(val))
#define norec_STM_WRITE_P(var, val)           norec_TxStore(norec_STM_SELF, \
                                                (vintp*)(void*)&(var), \
                                                norec_VP2IP(val))

#define norec_STM_LOCAL_WRITE(var, val)       ({var = val; var;})
#define norec_STM_LOCAL_WRITE_D(var, val)     ({var = val; var;})
#define norec_STM_LOCAL_WRITE_P(var, val)     ({var = val; var;})

#define norec_HTM_INC_CLOCK()                   norec_TxIncClock()


#endif /* STM_H */


/* =============================================================================
 *
 * End of stm.h
 *
 * =============================================================================
 */
