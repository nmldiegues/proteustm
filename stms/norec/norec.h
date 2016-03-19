/* =============================================================================
 *
 * norec.h
 *
 * Transactional Locking 2 software transactional memory
 *
 * =============================================================================
 *
 * Copyright (C) Sun Microsystems Inc., 2006.  All Rights Reserved.
 * Authors: Dave Dice, Nir Shavit, Ori Shalev.
 *
 * NOREC: Transactional Locking for Disjoint Access Parallelism
 *
 * Transactional Locking II,
 * Dave Dice, Ori Shalev, Nir Shavit
 * DISC 2006, Sept 2006, Stockholm, Sweden.
 *
 * =============================================================================
 *
 * Modified by Chi Cao Minh (caominh@stanford.edu)
 *
 * See VERSIONS for revision history
 *
 * =============================================================================
 */

#ifndef norec_NOREC_H
#define norec_NOREC_H 1














#include <stdint.h>


#  include <setjmp.h>






typedef struct norec__Thread norec_Thread;

#ifdef __cplusplus
extern "C" {
#endif





#  include <setjmp.h>

extern void reset_nesting_level();
extern void _ITM_siglongjmp(int val, sigjmp_buf env) __attribute__ ((noreturn));

#  define norec_SIGSETJMP(env, savesigs)      sigsetjmp(env, 0)
#  define norec_SIGLONGJMP(env, val)          reset_nesting_level(); _ITM_siglongjmp(val, env); assert(0) //siglongjmp(env, val); assert(0)



/*
 * Prototypes
 */




void     norec_TxStart       (norec_Thread*, sigjmp_buf*);

norec_Thread*  norec_TxNewThread   ();


inline unsigned norec_map_address_to_hash_set_idx(intptr_t* address);


void     norec_TxFreeThread  (norec_Thread*);
void     norec_TxInitThread  (norec_Thread*, long id);
int      norec_TxCommit      (norec_Thread*);
int      norec_TxCommitSTM   (norec_Thread*);
void     norec_TxAbort       (norec_Thread*);

intptr_t norec_TxLoad_inner        (norec_Thread*, volatile intptr_t*);
inline intptr_t norec_TxLoad        (norec_Thread*, volatile intptr_t*);
inline uint8_t norec_TxLoad_U8        (norec_Thread*, uint8_t*);
inline uint16_t norec_TxLoad_U16        (norec_Thread*, uint16_t*);
inline uint32_t norec_TxLoad_U32        (norec_Thread*, uint32_t*);
inline uint64_t norec_TxLoad_U64        (norec_Thread*, uint64_t*);

void     norec_TxStore_inner       (norec_Thread*, volatile intptr_t*, intptr_t, intptr_t);
inline void     norec_TxStore       (norec_Thread*, volatile intptr_t*, intptr_t);
inline void     norec_TxStore_U8       (norec_Thread*, uint8_t*, uint8_t);
inline void     norec_TxStore_U16       (norec_Thread*, uint16_t*, uint16_t);
inline void     norec_TxStore_U32       (norec_Thread*, uint32_t*, uint32_t);
inline void     norec_TxStore_U64       (norec_Thread*, uint64_t*, uint64_t);

void     norec_TxStoreLocal  (norec_Thread*, volatile intptr_t*, intptr_t);
void     norec_TxOnce        ();
void     norec_TxShutdown    ();

void*    norec_TxAlloc       (norec_Thread*, size_t);
void     norec_TxFree        (norec_Thread*, void*);

void     norec_TxIncClock    ();

long     norec_TxValidate    (norec_Thread*);
long     norec_TxFinalize    (norec_Thread*, long);
void     norec_TxResetAfterFinalize (norec_Thread*);




#ifdef __cplusplus
}
#endif


#endif /* NOREC_H */


/* =============================================================================
 *
 * End of norec.h
 *
 * =============================================================================
 */
