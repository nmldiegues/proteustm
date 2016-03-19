/* =============================================================================
 *
 * tl2.h
 *
 * Transactional Locking 2 software transactional memory
 *
 * =============================================================================
 *
 * Copyright (C) Sun Microsystems Inc., 2006.  All Rights Reserved.
 * Authors: Dave Dice, Nir Shavit, Ori Shalev.
 *
 * TL2: Transactional Locking for Disjoint Access Parallelism
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

#ifndef TL2_H
#define TL2_H 1














#include <stdint.h>
#include "tmalloc.h"


#  include <setjmp.h>




typedef uintptr_t      vwLock;  /* (Version,LOCKBIT) */


typedef struct _Thread Thread;

#ifdef __cplusplus
extern "C" {
#endif





#  include <setjmp.h>

extern void reset_nesting_level();
extern void _ITM_siglongjmp(int val, sigjmp_buf env) __attribute__ ((noreturn));

#  define SIGSETJMP(env, savesigs)      sigsetjmp(env, 0)
#  define SIGLONGJMP(env, val)          reset_nesting_level(); _ITM_siglongjmp(val, env); assert(0)



/*
 * Prototypes
 */




void     TxStart       (Thread*, sigjmp_buf*, int*);

Thread*  TxNewThread   ();


static int TrackLoad (Thread*, volatile vwLock*);


void     TxFreeThread  (Thread*);
void     TxInitThread  (Thread*, long id);
int      TxCommit      (Thread*);


int TxCommitNoAbortHTM (Thread*);
int TxCommitNoAbortSTM (Thread*);
void AfterCommit (Thread*);

void     TxAbort       (Thread*);

intptr_t TxLoad_inner        (Thread*, volatile intptr_t*);
inline intptr_t TxLoad        (Thread*, volatile intptr_t*);
inline uint8_t TxLoad_U8        (Thread*, uint8_t*);
inline uint16_t TxLoad_U16        (Thread*, uint16_t*);
inline uint32_t TxLoad_U32        (Thread*, uint32_t*);
inline uint64_t TxLoad_U64        (Thread*, uint64_t*);

void     TxStore_inner       (Thread*, volatile intptr_t*, intptr_t, intptr_t);
inline void     TxStore       (Thread*, volatile intptr_t*, intptr_t);
inline void     TxStore_U8       (Thread*, uint8_t*, uint8_t);
inline void     TxStore_U16       (Thread*, uint16_t*, uint16_t);
inline void     TxStore_U32       (Thread*, uint32_t*, uint32_t);
inline void     TxStore_U64       (Thread*, uint64_t*, uint64_t);

void TxStoreHTM (Thread*, volatile intptr_t*, intptr_t, vwLock);

void     TxOnce        ();
void     TxShutdown    ();

void*    TxAlloc       (Thread*, size_t);
void     TxFree        (Thread*, void*);

void *tm_calloc (size_t n, size_t size);

vwLock
GVGenerateWV_GV4 (Thread* Self, vwLock maxv);

vwLock
GVGenerateWV_GV6 (Thread* Self, vwLock maxv);

#ifndef _GVCONFIGURATION
#  define _GVCONFIGURATION              6
#endif

#if _GVCONFIGURATION == 4
#  define _GVFLAVOR                     "GV4"
#  define GVGenerateWV                  GVGenerateWV_GV4
#endif

#if _GVCONFIGURATION == 5
#  define _GVFLAVOR                     "GV5"
#  define GVGenerateWV                  GVGenerateWV_GV5
#endif

#if _GVCONFIGURATION == 6
#  define _GVFLAVOR                     "GV6"
#  define GVGenerateWV                  GVGenerateWV_GV6
#endif




#ifdef __cplusplus
}
#endif


#endif /* TL2_H */


/* =============================================================================
 *
 * End of tl2.h
 *
 * =============================================================================
 */
