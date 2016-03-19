/* =============================================================================
 *
 * tl2.c
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


#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include "platform.h"
#include "tl2.h"
#include "tmalloc.h"
#include "util.h"

__INLINE__ long ReadSetCoherent (Thread*);
__INLINE__ long ReadSetCoherentPessimistic (Thread*);


enum tl2_config {
    TL2_INIT_WRSET_NUM_ENTRY = 1024,
    TL2_INIT_RDSET_NUM_ENTRY = 8192,
    TL2_INIT_LOCAL_NUM_ENTRY = 1024,
};

typedef enum {
    TIDLE       = 0, /* Non-transactional */
    TTXN        = 1, /* Transactional mode */
    TABORTING   = 3, /* aborting - abort pending */
    TABORTED    = 5, /* defunct - moribund txn */
    TCOMMITTING = 7,
} Modes;

typedef enum {
    LOCKBIT  = 1,
    NADA
} ManifestContants;


typedef int            BitMap;
typedef unsigned char  byte;

/* Read set and write-set log entry */
typedef struct _AVPair {
    struct _AVPair* Next;
    struct _AVPair* NextHM;
    struct _AVPair* Prev;
    volatile intptr_t* Addr;
    intptr_t Valu;
    intptr_t Mask;
    volatile vwLock* LockFor; /* points to the vwLock covering Addr */
    vwLock rdv;               /* read-version @ time of 1st read - observed */
    byte Held;
    struct _Thread* Owner;
    long Ordinal;
} AVPair;

typedef struct _Log {
    AVPair* List;
    AVPair* put;        /* Insert position - cursor */
    AVPair* tail;       /* CCM: Pointer to last valid entry */
    AVPair* end;        /* CCM: Pointer to last entry */
    long ovf;           /* Overflow - request to grow */
    BitMap BloomFilter; /* Address exclusion fast-path test */
} Log;

#define LOG_BYTES_IN_WORD 3
#define BYTES_IN_WORD (1 << LOG_BYTES_IN_WORD)
#define HASH_WRITE_SET_SIZE  4096
#define HASH_WRITE_SET_MASK (HASH_WRITE_SET_SIZE - 1)
#define WRITE_SET_THRESHOLD 100

struct _Thread {
    long UniqID;
    volatile long Mode;
    volatile long HoldsLocks; /* passed start of update */
    volatile long Retries;
    volatile vwLock rv;
    vwLock wv;
    vwLock abv;
    vwLock maxv;
    int* ROFlag;
    int IsRO;
    long Starts;
    long Aborts; /* Tally of # of aborts */
    unsigned long long rng;
    unsigned long long xorrng [1];
    void* memCache;
    tmalloc_t* allocPtr; /* CCM: speculatively allocated */
    tmalloc_t* freePtr;  /* CCM: speculatively free'd */
    Log rdSet;
    Log wrSet;
    AVPair* hash_write_set[HASH_WRITE_SET_SIZE];
    uint8_t hash_write_set_markers[HASH_WRITE_SET_SIZE];
    unsigned int using_hash_map;
    sigjmp_buf* envPtr;
};

#define MASK_64 0x7l

#define SHIFT_8  0
#define SHIFT_16 1
#define SHIFT_32 2

#define WORD64_ADDRESS(addr)		((intptr_t*)((uintptr_t)addr & ~MASK_64))

#define WORD64_32_POS(addr)			(((uintptr_t)addr & MASK_64) >> SHIFT_32)
#define WORD64_16_POS(addr)			(((uintptr_t)addr & MASK_64) >> SHIFT_16)
#define WORD64_8_POS(addr)			(((uintptr_t)addr & MASK_64) >> SHIFT_8)

#define LOG_DEFAULT_LOCK_EXTENT_SIZE_WORDS 2
#define LOCK_EXTENT (LOG_DEFAULT_LOCK_EXTENT_SIZE_WORDS + LOG_BYTES_IN_WORD)

typedef union read_write_conv_u {
	// basic sizes
	uint8_t  b8[16];
	uint16_t b16[8];
	uint32_t b32[4];
	uint64_t b64[2];

	// other types
	float f[4];
	double d[2];

	////////////////////
	// helper structs //
	////////////////////

	// read 16 bits
	struct {
		uint8_t pad8_1;
		uint16_t b16[4];
		uint8_t pad8_2[7];
	} mask_16;

	// read 32 bit word
	struct {
		uint8_t pad8_1;
		uint32_t b32[2];
		uint8_t pad8_2[7];
	} mask_32_1;

	struct {
		uint8_t pad8_1[2];
		uint32_t b32[2];
		uint8_t pad8_2[6];
	} mask_32_2;

	struct {
		uint8_t pad8_1[3];
		uint32_t b32[2];
		uint8_t pad8_2[5];
	} mask_32_3;

	// read 64 bit word
	struct {
		uint8_t  pad8_1;
		uint64_t b64;
		uint8_t  pad8_2[7];
	} mask_64_1;

	struct {
		uint8_t  pad8_1[2];
		uint64_t b64;
		uint8_t  pad8_2[6];
	} mask_64_2;

	struct {
		uint8_t  pad8_1[3];
		uint64_t b64;
		uint8_t  pad8_2[5];
	} mask_64_3;

	struct {
		uint8_t  pad8_1[4];
		uint64_t b64;
		uint8_t  pad8_2[4];
	} mask_64_4;

	struct {
		uint8_t  pad8_1[5];
		uint64_t b64;
		uint8_t  pad8_2[3];
	} mask_64_5;

	struct {
		uint8_t  pad8_1[6];
		uint64_t b64;
		uint8_t  pad8_2[2];
	} mask_64_6;

	struct {
		uint8_t  pad8_1[7];
		uint64_t b64;
		uint8_t  pad8_2;
	} mask_64_7;

} read_write_conv_t;

/* #############################################################################
 * GENERIC INFRASTRUCTURE
 * #############################################################################
 */

static pthread_key_t    global_key_self;
static struct sigaction global_act_oldsigbus;
static struct sigaction global_act_oldsigsegv;

/* CCM: misaligned address (0xFF bytes) to generate bus error / segfault */
#define TL2_USE_AFTER_FREE_MARKER       (-1)


#ifndef TL2_CACHE_LINE_SIZE
#  define TL2_CACHE_LINE_SIZE           (64)
#endif


/* =============================================================================
 * MarsagliaXORV
 *
 * Simplistlic low-quality Marsaglia SHIFT-XOR RNG.
 * Bijective except for the trailing mask operation.
 * =============================================================================
 */
__INLINE__ unsigned long long
MarsagliaXORV (unsigned long long x)
{
    if (x == 0) {
        x = 1;
    }
    x ^= x << 6;
    x ^= x >> 21;
    x ^= x << 7;
    return x;
}


/* =============================================================================
 * MarsagliaXOR
 *
 * Simplistlic low-quality Marsaglia SHIFT-XOR RNG.
 * Bijective except for the trailing mask operation.
 * =============================================================================
 */
__INLINE__ unsigned long long
MarsagliaXOR (unsigned long long* seed)
{
    unsigned long long x = MarsagliaXORV(*seed);
    *seed = x;
    return x;
}


/* =============================================================================
 * TSRandom
 * =============================================================================
 */
__INLINE__ unsigned long long
TSRandom (Thread* Self)
{
    return MarsagliaXOR(&Self->rng);
}


/* =============================================================================
 * AtomicAdd
 * =============================================================================
 */
__INLINE__ intptr_t
AtomicAdd (volatile intptr_t* addr, intptr_t dx)
{
    intptr_t v;
    for (v = *addr; CAS(addr, v, v+dx) != v; v = *addr) {}
    return (v+dx);
}


/* =============================================================================
 * AtomicIncrement
 * =============================================================================
 */
__INLINE__ intptr_t
AtomicIncrement (volatile intptr_t* addr)
{
    intptr_t v;
    for (v = *addr; CAS(addr, v, v+1) != v; v = *addr) {}
    return (v+1);
}


/* #############################################################################
 * GLOBAL VERSION-CLOCK MANAGEMENT
 * #############################################################################
 */

/*
 * Consider 4M alignment for LockTab so we can use large-page support.
 * Alternately, we could mmap() the region with anonymous DZF pages.
 */
#  define _TABSZ  (1<< 20)
static volatile vwLock LockTab[_TABSZ];

/*
 * We use GClock[32] as the global counter.  It must be the sole occupant
 * of its cache line to avoid false sharing.  Even so, accesses to
 * GCLock will cause significant cache coherence & communication costs
 * as it is multi-read multi-write.
 */
static volatile vwLock GClock[TL2_CACHE_LINE_SIZE];
#define _GCLOCK  GClock[32]


/* =============================================================================
 * GVInit
 * =============================================================================
 */
__INLINE__ void
GVInit ()
{
    _GCLOCK = 0;
}


/* =============================================================================
 * GVRead
 * =============================================================================
 */
__INLINE__ vwLock
GVRead (Thread* Self)
{
    return _GCLOCK;
}


/*
 * GVGenerateWV():
 *
 * Conceptually, we'd like to fetch-and-add _GCLOCK.  In practice, however,
 * that naive approach, while safe and correct, results in CAS contention
 * and SMP cache coherency-related performance penalties.  As such, we
 * use either various schemes (GV4,GV5 or GV6) to reduce traffic on _GCLOCK.
 *
 * Global Version-Clock invariant:
 * I1: The generated WV must be > any previously observed (read) R
 */

/* =============================================================================
 * GVGenerateWV_GV4
 *
 * The GV4 form of GVGenerateWV() does not have a CAS retry loop. If the CAS
 * fails then we have 2 writers that are racing, trying to bump the global
 * clock. One increment succeeded and one failed. Because the 2 writers hold
 * locks at the time we bump, we know that their write-sets don't intersect. If
 * the write-set of one thread intersects the read-set of the other then we know
 * that one will subsequently fail validation (either because the lock associated
 * with the read-set entry is held by the other thread, or because the other
 * thread already made the update and dropped the lock, installing the new
 * version #). In this particular case it's safe if two threads call
 * GVGenerateWV() concurrently and they both generate the same (duplicate) WV.
 * That is, if we have writers that concurrently try to increment the
 * clock-version and then we allow them both to use the same wv. The failing
 * thread can "borrow" the wv of the successful thread.
 * =============================================================================
 */
vwLock
GVGenerateWV_GV4 (Thread* Self, vwLock maxv)
{
	if (maxv == 0) {
		maxv = Self->maxv;
	}
    vwLock gv = _GCLOCK;
    vwLock wv = gv + 2;
    vwLock k = CAS(&_GCLOCK, gv, wv);
    if (k != gv) {
        wv = k;
    }
    Self->wv = wv;
    Self->maxv = wv;
    return wv;
}


/* =============================================================================
 * GVGenerateWV_GV5
 *
 * Simply compute WV = GCLOCK + 2.
 *
 *  This increases the false+ abort-rate but reduces cache-coherence traffic.
 *  We only increment _GCLOCK at abort-time and perhaps TxStart()-time.
 *  The rate at which _GCLOCK advances controls performance and abort-rate.
 *  That is, the rate at which _GCLOCK advances is really a performance
 *  concern--related to false+ abort rates--rather than a correctness issue.
 *
 *  CONSIDER: use MAX(_GCLOCK, Self->rv, Self->wv, maxv, VERSION(Self->abv))+2
 * =============================================================================
 */
vwLock
GVGenerateWV_GV5 (Thread* Self, vwLock maxv)
{
	if (maxv == 0) {
		maxv = Self->maxv;
	}
    vwLock wv = _GCLOCK + 2;
    if (maxv > wv) {
        wv = maxv + 2;
    }
    Self->wv = wv;
    Self->maxv = wv;
    return wv;
}


/* =============================================================================
 * GVGenerateWV_GV6
 *
 * Composite of GV4 and GV5
 *
 * Trade-off -- abort-rate vs SMP cache-coherence costs.
 *
 * TODO: make the frequence mask adaptive at runtime.
 * Let the abort-rate or abort:success ratio drive the mask.
 * =============================================================================
 */
vwLock
GVGenerateWV_GV6 (Thread* Self, vwLock maxv)
{
    long rnd = (long)MarsagliaXOR(Self->xorrng);
    if ((rnd & 0x1F) == 0) {
        return GVGenerateWV_GV4(Self, maxv);
    } else {
        return GVGenerateWV_GV5(Self, maxv);
    }
}


/* =============================================================================
 * GVAbort
 *
 * GV5 and GV6 admit single-threaded false+ aborts.
 *
 * Consider the following scenario:
 *
 * GCLOCK is initially 10.  TxStart() fetches GCLOCK, observing 10, and
 * sets RV accordingly.  The thread calls TXST().  At commit-time the thread
 * computes WV = 12 in GVComputeWV().  T1 stores WV (12) in various versioned
 * lock words covered by the write-set.  The transaction commits successfully.
 * The thread then runs a 2nd txn. TxStart() fetches _GCLOCK == 12 and sets RV
 * accordingly.  The thread then calls TXLD() to fetch a variable written in the
 * 1st txn and observes Version# == 12, which is > RV.  The thread aborts.
 * This is false+ abort as there is no actual interference.
 * We can recover by incrementing _GCLOCK at abort-time if we find
 * that RV == GCLOCK and Self->Abv > GCLOCK.
 *
 * Alternately we can attempt to avoid the false+ abort by advancing
 * _GCLOCK at GVRead()-time if we find that the thread's previous WV is >
 * than the current _GCLOCK value
 * =============================================================================
 */
__INLINE__ long
GVAbort (Thread* Self)
{
#if _GVCONFIGURATION != 4
    vwLock abv = Self->abv;
    if (abv & LOCKBIT) {
        return 0; /* normal interference */
    }
    vwLock gv = _GCLOCK;
    if (Self->rv == gv && abv > gv) {
        CAS(&_GCLOCK, gv, abv); /* advance to either (gv+2) or abv */
        /* If this was a GV5/GV6-specific false+ abort then do not delay */
        return 1; /* false+ abort */
    }
#endif
    return 0; /* normal interference */
}


/* #############################################################################
 * TL2 INFRASTRUCTURE
 * #############################################################################
 */

volatile long StartTally         = 0;
volatile long AbortTally         = 0;
volatile long ReadOverflowTally  = 0;
volatile long WriteOverflowTally = 0;
volatile long LocalOverflowTally = 0;
#define TL2_TALLY_MAX          (((unsigned long)(-1)) >> 1)


/*
 * With PS the versioned lock words (the LockTab array) are table stable and
 * references will never fault.  Under PO, however, fetches by a doomed
 * zombie txn can fault if the referent is free()ed and unmapped
 */
#  define LDLOCK(a)                     *(a)     /* for PS */


/*
 * We use a degenerate Bloom filter with only one hash function generating
 * a single bit.  A traditional Bloom filter use multiple hash functions and
 * multiple bits.  Relatedly, the size our filter is small, so it can saturate
 * and become useless with a rather small write-set.
 * A better solution might be small per-thread hash tables keyed by address that
 * point into the write-set.
 * Beware that 0x1F == (MIN(sizeof(int),sizeof(intptr_t))*8)-
 */

#define FILTERHASH(a)                   ((UNS(a) >> 2) ^ (UNS(a) >> 5))
#define FILTERBITS(a)                   (1 << (FILTERHASH(a) & 0x1F))

/*
 * PSLOCK: maps variable address to lock address.
 * For PW the mapping is simply (UNS(addr)+sizeof(int))
 * COLOR attempts to place the lock(metadata) and the data on
 * different D$ indexes.
 */

#  define TABMSK                        (_TABSZ-1)

/*
#define COLOR                           0
#define COLOR                           (256-16)
*/
#define COLOR                           (128)

/*
 * Alternate experimental mapping functions ....
 * #define PSLOCK(a)     (LockTab + 0)                                   // PS1
 * #define PSLOCK(a)     (LockTab + ((UNS(a) >> 2) & 0x1FF))             // S512
 * #define PSLOCK(a)     (LockTab + (((UNS(a) >> 2) & (TABMSK & ~0x7)))) // PS1M
 * #define PSLOCK(a)     (LockTab + (((UNS(a) >> 6) & (TABMSK & ~0x7)))) // PS1M
 * #define PSLOCK(a)     (LockTab + ((((UNS(a) >> 2)|0xF) & TABMSK)))    // PS1M
 * #define PSLOCK(a)     (LockTab + (-(UNS(a) >> 2) & TABMSK))
 * #define PSLOCK(a)     (LockTab + ((UNS(a) >> 6) & TABMSK))
 * #define PSLOCK(a)     (LockTab + ((UNS(a) >> 2) & TABMSK))            // PS1
 */

/*
 * ILP32 vs LP64.  PSSHIFT == Log2(sizeof(intptr_t)).
 */
#define PSSHIFT                         ((sizeof(void*) == 4) ? 2 : 3)

#  define PSLOCK(a) (LockTab + (((UNS(a)+COLOR) >> PSSHIFT) & TABMSK)) /* PS1M */

/*
 * CCM: for debugging
 */
volatile vwLock*
pslock (volatile intptr_t* Addr)
{
    return PSLOCK(Addr);
}


/* =============================================================================
 * MakeList
 *
 * Allocate the primary list as a large chunk so we can guarantee ascending &
 * adjacent addresses through the list. This improves D$ and DTLB behavior.
 * =============================================================================
 */
__INLINE__ AVPair*
MakeList (long sz, Thread* Self)
{
    AVPair* ap = (AVPair*) malloc((sizeof(*ap) * sz) + TL2_CACHE_LINE_SIZE);
    assert(ap);
    memset(ap, 0, sizeof(*ap) * sz);
    AVPair* List = ap;
    AVPair* Tail = NULL;
    long i;
    for (i = 0; i < sz; i++) {
        AVPair* e = ap++;
        e->Next    = ap;
        e->Prev    = Tail;
        e->Owner   = Self;
        e->Ordinal = i;
        Tail = e;
    }
    Tail->Next = NULL;

    return List;
}


/* =============================================================================
 * FreeList
 * =============================================================================
 */
 void FreeList (Log*, long) __attribute__ ((noinline));
/*__INLINE__*/ void
FreeList (Log* k, long sz)
{
    /* Free appended overflow entries first */
    AVPair* e = k->end;
    if (e != NULL) {
        while (e->Ordinal >= sz) {
            AVPair* tmp = e;
            e = e->Prev;
            free(tmp);
        }
    }

    /* Free continguous beginning */
    free(k->List);
}


/* =============================================================================
 * ExtendList
 *
 * Postpend at the tail. We want the front of the list, which sees the most
 * traffic, to remains contiguous.
 * =============================================================================
 */
__INLINE__ AVPair*
ExtendList (AVPair* tail)
{
    AVPair* e = (AVPair*)malloc(sizeof(*e));
    assert(e);
    memset(e, 0, sizeof(*e));
    tail->Next = e;
    e->Prev    = tail;
    e->Next    = NULL;
    e->Owner   = tail->Owner;
    e->Ordinal = tail->Ordinal + 1;
    /*e->Held    = 0; -- done by memset*/
    return e;
}

inline intptr_t MaskWord(intptr_t old, intptr_t val, intptr_t mask) {
	if(mask == ~0x0) {
		return val;
	}
	return (old & ~mask) | (val & mask);
}

/* =============================================================================
 * WriteBackForward
 *
 * Transfer the data in the log its ultimate location.
 * =============================================================================
 */
__INLINE__ void
WriteBackForward (Log* k)
{
    AVPair* e;
    AVPair* End = k->put;
    for (e = k->List; e != End; e = e->Next) {
    	*(e->Addr) = MaskWord(*(e->Addr), e->Valu, e->Mask);
    }
}


/* =============================================================================
 * FindFirst
 *
 * Search for first log entry that contains lock.
 * =============================================================================
 */
__INLINE__ AVPair*
FindFirst (Log* k, volatile vwLock* Lock)
{
    AVPair* e;
    AVPair* const End = k->put;
    for (e = k->List; e != End; e = e->Next) {
        if (e->LockFor == Lock) {
            return e;
        }
    }
    return NULL;
}

inline unsigned map_address_to_hash_set_idx(intptr_t* address) {
    return (((uintptr_t)address) >> LOCK_EXTENT) & HASH_WRITE_SET_MASK;
}

/* =============================================================================
 * RecordStore
 * =============================================================================
 */

__INLINE__ void
RecordStore (Thread* Self, Log* k, volatile intptr_t* Addr, intptr_t Valu, volatile vwLock* Lock, intptr_t mask)
{
    AVPair* e = k->put;
    if (e == NULL) {
        k->ovf++;
        e = ExtendList(k->tail);
        k->end = e;
    }
    ASSERT(Addr != NULL);
    k->tail    = e;
    k->put     = e->Next;
    e->Addr    = Addr;
    e->Valu    = Valu;
    e->LockFor = Lock;
    e->Held    = 0;
    e->Mask    = mask;
    e->rdv     = LOCKBIT; /* use either 0 or LOCKBIT */

    if (Self->using_hash_map) {
        unsigned hash_write_set_idx = map_address_to_hash_set_idx(Addr);
        if(Self->hash_write_set_markers[hash_write_set_idx]) {
            AVPair* first_log_entry = Self->hash_write_set[hash_write_set_idx];
            e->NextHM = first_log_entry;
            Self->hash_write_set[hash_write_set_idx] = e;
        } else {
            Self->hash_write_set_markers[hash_write_set_idx] = 1;
            Self->hash_write_set[hash_write_set_idx] = e;
            e->NextHM = NULL;
        }
    }

}

inline void TxStore (Thread* Self, volatile intptr_t* addr, intptr_t valu) {
	    volatile vwLock* LockFor;
	    vwLock rdv;

	    /*
	     * In TL2 we're always coherent, so we should never see NULL stores.
	     * In TL it was possible to see NULL stores in zombie txns.
	     */

	    ASSERT(Self->Mode == TTXN);
	    if (Self->IsRO) {
	        *(Self->ROFlag) = 0;
	        TxAbort(Self);
	        return;
	    }

	  LockFor = PSLOCK(addr);

	    /*
	     * CONSIDER: spin briefly (bounded) while the object is locked,
	     * periodically calling ReadSetCoherent(Self)
	     */

	    rdv = LDLOCK(LockFor);

	    Log* wr = &Self->wrSet;
	    /*
	     * Convert a redundant "idempotent" store to a tracked load.
	     * This helps minimize the wrSet size and reduces false+ aborts.
	     * Conceptually, "a = x" is equivalent to "if (a != x) a = x"
	     * This is entirely optional
	     */
	    MEMBARLDLD();

	    if (ALWAYS && LDNF(addr) == valu) {
	        AVPair* e;
	        for (e = wr->tail; e != NULL; e = e->Prev) {
	            ASSERT(e->Addr != NULL);
	            if (e->Addr == addr) {
	                ASSERT(LockFor == e->LockFor);
	                e->Valu = valu; /* CCM: update associated value in write-set */
	                return;
	            }
	        }
	        /* CCM: Not writing new value; convert to load */
	        if ((rdv & LOCKBIT) == 0 && rdv <= Self->rv && LDLOCK(LockFor) == rdv) {
	            if (!TrackLoad(Self, LockFor)) {
	                TxAbort(Self);
	            }
	            return;
	        }
	    }

	    wr->BloomFilter |= FILTERBITS(addr);
	    RecordStore(Self, wr, addr, valu, LockFor, ~0x0);
}

inline void TxStore_U8 (Thread* thread, uint8_t* addr, uint8_t value) {
	unsigned pos = ((uintptr_t)addr & MASK_64) >> SHIFT_8;
	read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b8[pos] = ~0;
	read_write_conv_t val;
	val.b8[pos] = value;
	TxStore_inner(thread, (intptr_t*)(((uintptr_t)addr) & ~MASK_64), (intptr_t)val.b64[0], (intptr_t)mask.b64[0]);
}

inline void TxStore_U16 (Thread* thread, uint16_t* addr, uint16_t value) {
	unsigned pos = ((uintptr_t)addr & MASK_64) >> SHIFT_16;
	read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b16[pos] = ~0;
	read_write_conv_t val;
	val.b16[pos] = value;
	TxStore_inner(thread, (intptr_t*)(((uintptr_t)addr) & ~MASK_64), (intptr_t)val.b64[0], (intptr_t)mask.b64[0]);
}

inline void TxStore_U32 (Thread* thread, uint32_t* addr, uint32_t value) {
	unsigned pos = ((uintptr_t)addr & MASK_64) >> SHIFT_32;
	read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b32[pos] = ~0;
	read_write_conv_t val;
	val.b32[pos] = value;
	TxStore_inner(thread, (intptr_t*)(((uintptr_t)addr) & ~MASK_64), (intptr_t)val.b64[0], (intptr_t)mask.b64[0]);
}

inline void TxStore_U64 (Thread* thread, uint64_t* addr, uint64_t value) {
	TxStore_inner(thread, (intptr_t*)addr, (intptr_t)value, (intptr_t)~0x0);
}

/* =============================================================================
 * TrackLoad
 * =============================================================================
 */
__INLINE__ int
TrackLoad (Thread* Self, volatile vwLock* LockFor)
{
    Log* k = &Self->rdSet;
    AVPair* e = k->put;
    if (e == NULL) {
        k->ovf++;
        e = ExtendList(k->tail);
        k->end = e;
    }

    k->tail    = e;
    k->put     = e->Next;
    e->LockFor = LockFor;
    /* Note that Valu and Addr fields are undefined for tracked loads */

    return 1;
}

void TxOnce () {
    CTASSERT((_TABSZ & (_TABSZ-1)) == 0); /* must be power of 2 */

    GVInit();
    printf("TL2 system ready: GV=%s\n", _GVFLAVOR);

    pthread_key_create(&global_key_self, NULL); /* CCM: do before we register handler */

}

void TxShutdown () {
    printf("TL2 system shutdown:\n"
           "  GCLOCK=0x%lX Starts=%li Aborts=%li\n"
           "  Overflows: R=%li W=%li L=%li\n",
           (unsigned long)_GCLOCK, StartTally, AbortTally,
           ReadOverflowTally, WriteOverflowTally, LocalOverflowTally);

    pthread_key_delete(global_key_self);

    MEMBARSTLD();
}

Thread* TxNewThread () {
    Thread* t = (Thread*)malloc(sizeof(Thread));
    assert(t);

    return t;
}

void TxFreeThread (Thread* t) {
    AtomicAdd((volatile intptr_t*)((void*)(&ReadOverflowTally)),  t->rdSet.ovf);

    long wrSetOvf = 0;
    Log* wr;
    wr = &t->wrSet;
    {
        wrSetOvf += wr->ovf;
    }
    AtomicAdd((volatile intptr_t*)((void*)(&WriteOverflowTally)), wrSetOvf);

    AtomicAdd((volatile intptr_t*)((void*)(&StartTally)),         t->Starts);
    AtomicAdd((volatile intptr_t*)((void*)(&AbortTally)),         t->Aborts);

    tmalloc_free(t->allocPtr);
    tmalloc_free(t->freePtr);

    FreeList(&(t->rdSet),     TL2_INIT_RDSET_NUM_ENTRY);
    FreeList(&(t->wrSet),     TL2_INIT_WRSET_NUM_ENTRY);

    free(t);
}

void TxInitThread (Thread* t, long id) {
    /* CCM: so we can access TL2's thread metadata in signal handlers */
    pthread_setspecific(global_key_self, (void*)t);

    memset(t, 0, sizeof(*t));     /* Default value for most members */

    t->UniqID = id;
    t->rng = id + 1;
    t->xorrng[0] = t->rng;

    t->wrSet.List = MakeList(TL2_INIT_WRSET_NUM_ENTRY, t);
    t->wrSet.put = t->wrSet.List;

    t->rdSet.List = MakeList(TL2_INIT_RDSET_NUM_ENTRY, t);
    t->rdSet.put = t->rdSet.List;

    t->allocPtr = tmalloc_alloc(1);
    assert(t->allocPtr);
    t->freePtr = tmalloc_alloc(1);
    assert(t->freePtr);
}

__INLINE__ void txReset (Thread* Self) {
    Self->Mode = TIDLE;

    Self->wrSet.put = Self->wrSet.List;
    Self->wrSet.tail = NULL;

    Self->wrSet.BloomFilter = 0;
    Self->rdSet.put = Self->rdSet.List;
    Self->rdSet.tail = NULL;

    Self->HoldsLocks = 0;

    if (Self->using_hash_map) {
        memset(Self->hash_write_set_markers, 0, sizeof(uint8_t) * HASH_WRITE_SET_SIZE);
    }
    Self->using_hash_map = 1;
}

__INLINE__ void txCommitReset (Thread* Self) {
    txReset(Self);
    Self->Retries = 0;
}

__INLINE__ long ReadSetCoherent (Thread* Self) {
    intptr_t dx = 0;
    vwLock rv = Self->rv;
    Log* const rd = &Self->rdSet;
    AVPair* const EndOfList = rd->put;
    AVPair* e;

    ASSERT((rv & LOCKBIT) == 0);

    for (e = rd->List; e != EndOfList; e = e->Next) {
        ASSERT(e->LockFor != NULL);
        vwLock v = LDLOCK(e->LockFor);
        if (v & LOCKBIT) {
            dx |= UNS(((AVPair*)(UNS(v) & ~LOCKBIT))->Owner) ^ UNS(Self);
        } else {
            dx |= (v > rv);
        }
    }

    return (dx == 0);
}


__INLINE__ void RestoreLocks (Thread* Self)
{
    Log* wr = &Self->wrSet;
    {
        AVPair* p;
        AVPair* const End = wr->put;
        for (p = wr->List; p != End; p = p->Next) {
            ASSERT(p->Addr != NULL);
            ASSERT(p->LockFor != NULL);
            if (p->Held == 0) {
                continue;
            }
            ASSERT(OwnerOf(*(p->LockFor)) == Self);
            ASSERT(*(p->LockFor) == (UNS(p)|LOCKBIT));
            ASSERT((p->rdv & LOCKBIT) == 0);
            p->Held = 0;
            *(p->LockFor) = p->rdv;
        }
    }
    Self->HoldsLocks = 0;
}

__INLINE__ void DropLocks (Thread* Self, vwLock wv) {
    Log* wr = &Self->wrSet;
    {
        AVPair* p;
        AVPair* const End = wr->put;
        for (p = wr->List; p != End; p = p->Next) {
            ASSERT(p->Addr != NULL);
            ASSERT(p->LockFor != NULL);
            if (p->Held == 0) {
                continue;
            }
            p->Held = 0;
#  if _GVCONFIGURATION == 4
            ASSERT(wv > p->rdv);
#  else
            /* GV5 and GV6 admit wv == p->rdv */
            ASSERT(wv >= p->rdv);
#  endif
            ASSERT(OwnerOf(*(p->LockFor)) == Self);
            ASSERT(*(p->LockFor) == (UNS(p)|LOCKBIT));
            *(p->LockFor) = wv;
        }
    }
    Self->HoldsLocks = 0;
}


__INLINE__ void backoff (Thread* Self, long attempt) {
#ifdef TL2_BACKOFF_EXPONENTIAL
    unsigned long long n = 1 << ((attempt < 63) ? (attempt) : (63));
    unsigned long long stall = TSRandom(Self) % n;
#else
    unsigned long long stall = TSRandom(Self) & 0xF;
    stall += attempt >> 2;
    stall *= 10;
#endif
    /* CCM: timer function may misbehave */
    volatile typeof(stall) i = 0;
    while (i++ < stall) {
        PAUSE();
    }
}

__INLINE__ long TryFastUpdate (Thread* Self) {
    Log* const wr = &Self->wrSet;
    Log* const rd = &Self->rdSet;
    long ctr;
    vwLock wv;

    ASSERT(Self->Mode == TTXN);


    /*
     * Lock-acquisition phase ...
     *
     * CONSIDER: While iterating over the locks that cover the write-set
     * track the maximum observed version# in maxv.
     * In GV4:   wv = GVComputeWV(); ASSERT wv > maxv
     * In GV5|6: wv = GVComputeWV(); if (maxv >= wv) wv = maxv + 2
     * This is strictly an optimization.
     * maxv isn't required for algorithmic correctness
     */
    Self->HoldsLocks = 1;
    ctr = 1000; /* Spin budget - TUNABLE */
    vwLock maxv = 0;
    AVPair* p;
    {
        AVPair* const End = wr->put;
        for (p = wr->List; p != End; p = p->Next) {
            volatile vwLock* const LockFor = p->LockFor;
            vwLock cv;
            prefetchw(LockFor);
            cv = LDLOCK(LockFor);
            //int readVar = FindFirst(rd, LockFor) != NULL;
            if ((cv & LOCKBIT) && ((AVPair*)(cv ^ LOCKBIT))->Owner == Self) {
                /* CCM: revalidate read because could be a hash collision */
                /*if (readVar) {
                    if (((AVPair*)(cv ^ LOCKBIT))->rdv > Self->rv) {
                        Self->abv = cv;
                        return 0;
                    }
                }*/
                /* Already locked by an earlier iteration. */
                continue;
            }

            /* SIGTM does not maintain a read set */
            //if (readVar) {
                /*
                 * READ-WRITE stripe
                 */
                if ((cv & LOCKBIT) == 0 &&
                    cv <= Self->rv &&
                    UNS(CAS(LockFor, cv, (UNS(p)|UNS(LOCKBIT)))) == UNS(cv))
                {
                    if (cv > maxv) {
                        maxv = cv;
                    }
                    p->rdv  = cv;
                    p->Held = 1;
                    continue;
                }
                /*
                 * The stripe is either locked or the previously observed read-
                 * version changed.  We must abort. Spinning makes little sense.
                 * In theory we could spin if the read-version is the same but
                 * the lock is held in the faint hope that the owner might
                 * abort and revert the lock
                 */
                Self->abv = cv;
                return 0;
            /*} else
            {
#      ifdef TL2_NOCM
                long c = 0;
#      else
                long c = ctr;
#      endif
                for (;;) {
                    cv = LDLOCK(LockFor);
                    if ((cv & LOCKBIT) == 0 &&
                        UNS(CAS(LockFor, cv, (UNS(p)|UNS(LOCKBIT)))) == UNS(cv))
                    {
                        if (cv > maxv) {
                            maxv = cv;
                        }
                        p->rdv  = cv;
                        p->Held = 1;
                        break;
                    }
                    if (--c < 0) {
                        return 0;
                    }
                    PAUSE();
                }
            } */ /* write-only stripe */
        } /* foreach (entry in write-set) */
    }

    wv = GVGenerateWV(Self, maxv);

    /*
     * We now hold all the locks for RW and W objects.
     * Next we validate that the values we've fetched from pure READ objects
     * remain coherent.
     *
     * If GVGenerateWV() is implemented as a simplistic atomic fetch-and-add
     * then we can optimize by skipping read-set validation in the common-case.
     * Namely,
     *   if (Self->rv != (wv-2) && !ReadSetCoherent(Self)) { ... abort ... }
     * That is, we could elide read-set validation for pure READ objects if
     * there were no intervening write txns between the fetch of _GCLOCK into
     * Self->rv in TxStart() and the increment of _GCLOCK in GVGenerateWV()
     */

    /*
     * CCM: for SIGTM, the read filter would have triggered an abort already
     * if the read-set was not consistent.
     */
    if (!ReadSetCoherent(Self)) {
        /*
         * The read-set is inconsistent.
         * The transaction is spoiled as the read-set is stale.
         * The candidate results produced by the txn and held in
         * the write-set are a function of the read-set, and thus invalid
         */
        return 0;
    }

    /*
     * We are now committed - this txn is successful.
     */

    {
        WriteBackForward(wr); /* write-back the deferred stores */
    }
    MEMBARSTST(); /* Ensure the above stores are visible  */
    DropLocks(Self, wv); /* Release locks and increment the version */

    /*
     * Ensure that all the prior STs have drained before starting the next
     * txn.  We want to avoid the scenario where STs from "this" txn
     * languish in the write-buffer and inadvertently satisfy LDs in
     * a subsequent txn via look-aside into the write-buffer
     */
    MEMBARSTLD();

    return 1; /* success */
}



/* =============================================================================
 * TxAbort
 *
 * Our mechanism admits mutual abort with no progress - livelock.
 * Consider the following scenario where T1 and T2 execute concurrently:
 * Thread T1:  WriteLock A; Read B LockWord; detect locked, abort, retry
 * Thread T2:  WriteLock B; Read A LockWord; detect locked, abort, retry
 *
 * Possible solutions:
 *
 * - Try backoff (random and/or exponential), with some mixture
 *   of yield or spinning.
 *
 * - Use a form of deadlock detection and arbitration.
 *
 * In practice it's likely that a semi-random semi-exponential back-off
 * would be best.
 * =============================================================================
 */
void
TxAbort (Thread* Self)
{
    Self->Mode = TABORTED;

    if (Self->HoldsLocks) {
        RestoreLocks(Self);
    }

    Self->Retries++;
    Self->Aborts++;

    if (GVAbort(Self)) {
        /* possibly advance _GCLOCK for GV5 or GV6 */
        goto __rollback;
    }

    /*
     * Beware: back-off is useful for highly contended environments
     * where N threads shows negative scalability over 1 thread.
     * Extreme back-off restricts parallelism and, in the extreme,
     * is tantamount to allowing the N parallel threads to run serially
     * 1 at-a-time in succession.
     *
     * Consider: make the back-off duration a function of:
     * - a random #
     * - the # of previous retries
     * - the size of the previous read-set
     * - the size of the previous write-set
     *
     * Consider using true CSMA-CD MAC style random exponential backoff
     */

#ifndef TL2_NOCM
    if (Self->Retries > 3) { /* TUNABLE */
        backoff(Self, Self->Retries);
    }
#endif

__rollback:

    tmalloc_releaseAllReverse(Self->allocPtr, NULL);
    tmalloc_clear(Self->freePtr);


    txReset(Self);
    Self->rv = GVRead(Self);
    ASSERT((Self->rv & LOCKBIT) == 0);
    MEMBARLDLD();
    Self->Mode = TTXN;
    ASSERT(Self->wrSet.put == Self->wrSet.List);
    Self->Starts++;

    SIGLONGJMP(*Self->envPtr, 1);
    ASSERT(0);
}


/* =============================================================================
 * TxStore
 * =============================================================================
 */
void TxStore_inner (Thread* Self, volatile intptr_t* addr, intptr_t valu, intptr_t mask) {
    volatile vwLock* LockFor;
    LockFor = PSLOCK(addr);
    Log* wr = &Self->wrSet;
    wr->BloomFilter |= FILTERBITS(addr);
    RecordStore(Self, wr, addr, valu, LockFor, mask);
}

void TxStoreHTM (Thread* Self, volatile intptr_t* addr, intptr_t valu, vwLock clockNext)
{
    volatile vwLock* LockFor;
    vwLock rdv;
	LockFor = PSLOCK(addr);
	*(LockFor) = clockNext;
}

/* =============================================================================
 * TxLoad
 * =============================================================================
 */
intptr_t TxLoad_inner (Thread* Self, volatile intptr_t* Addr) {
    intptr_t Valu;
    intptr_t msk = FILTERBITS(Addr);
    if ((Self->wrSet.BloomFilter & msk) == msk) {

        if (Self->using_hash_map == 0) {
            unsigned long counter = 0;
            Log* wr = &Self->wrSet;
            AVPair* e;
            unsigned char found = 0;
            for (e = wr->tail; e != NULL; e = e->Prev, counter++) {
                if (e->Addr == Addr) {
                    found = 1;
                    break;
                }
            }
            if (counter > WRITE_SET_THRESHOLD) {
                AVPair* End = wr->put;
                for (e = wr->List; e != End; e = e->Next) {
                    unsigned hash_write_set_idx = map_address_to_hash_set_idx(e->Addr);
                    if(Self->hash_write_set_markers[hash_write_set_idx]) {
                        AVPair* first_log_entry = Self->hash_write_set[hash_write_set_idx];
                        e->NextHM = first_log_entry;
                    } else {
                        Self->hash_write_set_markers[hash_write_set_idx] = 1;
                        Self->hash_write_set[hash_write_set_idx] = e;
                        e->NextHM = NULL;
                    }
                }
                Self->using_hash_map = 1;
            }
            if (found) {
                return MaskWord(*(e->Addr), e->Valu, e->Mask);
            }
        } else {
            unsigned hash_write_set_idx = map_address_to_hash_set_idx(Addr);
            AVPair *first_log_entry, *log_entry;
            first_log_entry = log_entry = Self->hash_write_set[hash_write_set_idx];

            if(Self->hash_write_set_markers[hash_write_set_idx]) {
                while(log_entry != NULL) {
                    if(log_entry->Addr == Addr) {
                        return MaskWord(*(log_entry->Addr), log_entry->Valu, log_entry->Mask);
                    }
                    log_entry = log_entry->NextHM;
                }
            }
        }
    }

    volatile vwLock* LockFor = PSLOCK(Addr);
    vwLock rdv = LDLOCK(LockFor) & ~LOCKBIT;
    MEMBARLDLD();
    Valu = LDNF(Addr);
    MEMBARLDLD();
    if (rdv <= Self->rv && LDLOCK(LockFor) == rdv) {
        if (!Self->IsRO) {
            if (!TrackLoad(Self, LockFor)) {
                TxAbort(Self);
            }
        }
        return Valu;
    }

    Self->abv = rdv;
    TxAbort(Self);
    ASSERT(0);

    return 0;
}

inline intptr_t TxLoad (Thread* Self, volatile intptr_t* Addr) {

    intptr_t Valu;

    ASSERT(Self->Mode == TTXN);

    intptr_t msk = FILTERBITS(Addr);
    if ((Self->wrSet.BloomFilter & msk) == msk) {
        Log* wr = &Self->wrSet;
        AVPair* e;
        for (e = wr->tail; e != NULL; e = e->Prev) {
            ASSERT(e->Addr != NULL);
            if (e->Addr == Addr) {
                return e->Valu;
            }
        }
    }

    volatile vwLock* LockFor = PSLOCK(Addr);
    vwLock rdv = LDLOCK(LockFor) & ~LOCKBIT;
    MEMBARLDLD();
    Valu = LDNF(Addr);
    MEMBARLDLD();
    if (rdv <= Self->rv && LDLOCK(LockFor) == rdv) {
        if (!Self->IsRO) {
            if (!TrackLoad(Self, LockFor)) {
                TxAbort(Self);
            }
        }
        return Valu;
    }

    Self->abv = rdv;
    TxAbort(Self);
    ASSERT(0);

    return 0;
}

inline uint8_t TxLoad_U8 (Thread* thread, uint8_t* addr) {
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(TxLoad_inner(thread, WORD64_ADDRESS(addr)));
	return value.b8[WORD64_8_POS(addr)];
}

inline uint16_t TxLoad_U16 (Thread* thread, uint16_t* addr) {
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(TxLoad_inner(thread, WORD64_ADDRESS(addr)));
	return value.b16[WORD64_16_POS(addr)];
}

inline uint32_t TxLoad_U32 (Thread* thread, uint32_t* addr) {
	read_write_conv_t value;
	value.b64[0] = (uint64_t)(TxLoad_inner(thread, WORD64_ADDRESS(addr)));
	return value.b32[WORD64_32_POS(addr)];
}

inline uint64_t TxLoad_U64 (Thread* thread, uint64_t* addr) {
    return (uint64_t) TxLoad_inner(thread, (intptr_t*)addr);
}

void TxStart (Thread* Self, sigjmp_buf* envPtr, int* ROFlag) {
    txReset(Self);

    Self->rv = GVRead(Self);
    MEMBARLDLD();

    Self->Mode = TTXN;
    Self->ROFlag = ROFlag;
    Self->IsRO = ROFlag ? *ROFlag : 0;
    Self->envPtr= envPtr;

    Self->Starts++;
}

int TxCommit (Thread* Self) {
    /* Fast-path: Optional optimization for pure-readers */
    if (Self->wrSet.put == Self->wrSet.List)
    {
        /* Given TL2 the read-set is already known to be coherent. */
        txCommitReset(Self);
        tmalloc_clear(Self->allocPtr);
        return 1;
    }

    if (TryFastUpdate(Self)) {
        txCommitReset(Self);
        tmalloc_clear(Self->allocPtr);
        return 1;
    }

    TxAbort(Self);
    ASSERT(0);

    return 0;
}

int TxCommitNoAbortHTM (Thread* Self) {
    if (Self->wrSet.put == Self->wrSet.List)
    {
        return 1;
    }

    vwLock maxv = 0;
    vwLock rv = Self->rv;

    intptr_t dx = 0;
    Log* const rd = &Self->rdSet;
    AVPair* const EndOfList = rd->put;
    AVPair* e;
    for (e = rd->List; e != EndOfList; e = e->Next) {
        vwLock v = LDLOCK(e->LockFor);
        dx |= (v > rv);
        if (v > maxv) {
        	maxv = v;
        }
    }
    if (dx != 0) {
    	return 0;
    }

    vwLock wv = GVGenerateWV(Self, maxv);

    Log* const wr = &Self->wrSet;
    AVPair* const End = wr->put;
    for (e = wr->List; e != End; e = e->Next) {
    	*(e->LockFor) = wv;
    	*(e->Addr) = e->Valu;
    }

    return 1;
}

int TxCommitNoAbortSTM (Thread* Self) {
    if (Self->wrSet.put == Self->wrSet.List)
    {
        return 1;
    }

    if (TryFastUpdate(Self)) {
        return 1;
    }

    return 0;
}


void AfterCommit (Thread* Self)
{
    txCommitReset(Self);
    tmalloc_clear(Self->allocPtr);
}
