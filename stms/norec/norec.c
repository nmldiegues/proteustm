#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include "norec_platform.h"
#include "norec.h"
#include "norec_util.h"


norec___INLINE__ long norec_ReadSetCoherent (norec_Thread*);


enum norec_config {
    NOREC_INIT_WRSET_NUM_ENTRY = 1024,
    NOREC_INIT_RDSET_NUM_ENTRY = 8192,
    NOREC_INIT_LOCAL_NUM_ENTRY = 1024,
};


typedef int            norec_BitMap;

/* Read set and write-set log entry */
typedef struct norec__AVPair {
    struct norec__AVPair* Next;
    struct norec__AVPair* NextHM;
    struct norec__AVPair* Prev;
    volatile intptr_t* Addr;
    intptr_t Valu;
    intptr_t Mask;
    long Ordinal;
} norec_AVPair;

typedef struct norec__Log {
    norec_AVPair* List;
    norec_AVPair* put;        /* Insert position - cursor */
    norec_AVPair* tail;       /* CCM: Pointer to last valid entry */
    norec_AVPair* end;        /* CCM: Pointer to last entry */
    long ovf;           /* Overflow - request to grow */
    norec_BitMap BloomFilter; /* Address exclusion fast-path test */
} norec_Log;

#define NOREC_LOG_BYTES_IN_WORD 3
#define NOREC_BYTES_IN_WORD (1 << NOREC_LOG_BYTES_IN_WORD)
#define NOREC_HASH_WRITE_SET_SIZE  4096
#define NOREC_HASH_WRITE_SET_MASK (NOREC_HASH_WRITE_SET_SIZE - 1)
#define NOREC_WRITE_SET_THRESHOLD 100

struct norec__Thread {
    long UniqID;
    volatile long Retries;
    long Starts;
    long Aborts; /* Tally of # of aborts */
    long snapshot;
    unsigned long long rng;
    unsigned long long xorrng [1];
    norec_Log rdSet;
    norec_Log wrSet;
    norec_AVPair* norec_hash_write_set[NOREC_HASH_WRITE_SET_SIZE];
    uint8_t norec_hash_write_set_markers[NOREC_HASH_WRITE_SET_SIZE];
    unsigned int using_hash_map;
    sigjmp_buf* envPtr;
};

typedef struct
{
    long value;
    long padding1;
    long padding2;
    long padding3;
    long padding4;
    long padding5;
    long padding6;
    long padding7;
} norec_aligned_type_t ;

#define NOREC_MASK_64 0x7l

#define NOREC_SHIFT_8  0
#define NOREC_SHIFT_16 1
#define NOREC_SHIFT_32 2

#define NOREC_WORD64_ADDRESS(addr)		((intptr_t*)((uintptr_t)addr & ~NOREC_MASK_64))

#define NOREC_WORD64_32_POS(addr)			(((uintptr_t)addr & NOREC_MASK_64) >> NOREC_SHIFT_32)
#define NOREC_WORD64_16_POS(addr)			(((uintptr_t)addr & NOREC_MASK_64) >> NOREC_SHIFT_16)
#define NOREC_WORD64_8_POS(addr)			(((uintptr_t)addr & NOREC_MASK_64) >> NOREC_SHIFT_8)

#define NOREC_LOG_DEFAULT_LOCK_EXTENT_SIZE_WORDS 2
#define LOCK_EXTENT (NOREC_LOG_DEFAULT_LOCK_EXTENT_SIZE_WORDS + NOREC_LOG_BYTES_IN_WORD)

typedef union norec_read_write_conv_u {
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

} norec_read_write_conv_t;

__attribute__((aligned(64))) volatile norec_aligned_type_t* norec_LOCK;

static pthread_key_t    norec_global_key_self;

void norec_TxIncClock() {
    norec_LOCK->value = norec_LOCK->value + 2;
}

#ifndef NOREC_CACHE_LINE_SIZE
#  define NOREC_CACHE_LINE_SIZE           (64)
#endif

norec___INLINE__ unsigned long long
norec_MarsagliaXORV (unsigned long long x)
{
    if (x == 0) {
        x = 1;
    }
    x ^= x << 6;
    x ^= x >> 21;
    x ^= x << 7;
    return x;
}

norec___INLINE__ unsigned long long
norec_MarsagliaXOR (unsigned long long* seed)
{
    unsigned long long x = norec_MarsagliaXORV(*seed);
    *seed = x;
    return x;
}

norec___INLINE__ unsigned long long
norec_TSRandom (norec_Thread* Self)
{
    return norec_MarsagliaXOR(&Self->rng);
}

norec___INLINE__ intptr_t
norec_AtomicAdd (volatile intptr_t* addr, intptr_t dx)
{
    intptr_t v;
    for (v = *addr; norec_CAS(addr, v, v+dx) != v; v = *addr) {}
    return (v+dx);
}

volatile long norec_StartTally         = 0;
volatile long norec_AbortTally         = 0;
volatile long norec_ReadOverflowTally  = 0;
volatile long norec_WriteOverflowTally = 0;
volatile long norec_LocalOverflowTally = 0;
#define NOREC_TALLY_MAX          (((unsigned long)(-1)) >> 1)

#define norec_FILTERHASH(a)                   ((norec_UNS(a) >> 2) ^ (norec_UNS(a) >> 5))
#define norec_FILTERBITS(a)                   (1 << (norec_FILTERHASH(a) & 0x1F))

norec___INLINE__ norec_AVPair*
norec_MakeList (long sz, norec_Thread* Self)
{
    norec_AVPair* ap = (norec_AVPair*) malloc((sizeof(*ap) * sz) + NOREC_CACHE_LINE_SIZE);
    assert(ap);
    memset(ap, 0, sizeof(*ap) * sz);
    norec_AVPair* List = ap;
    norec_AVPair* Tail = NULL;
    long i;
    for (i = 0; i < sz; i++) {
        norec_AVPair* e = ap++;
        e->Next    = ap;
        e->Prev    = Tail;
        e->Ordinal = i;
        Tail = e;
    }
    Tail->Next = NULL;

    return List;
}

 void norec_FreeList (norec_Log*, long) __attribute__ ((noinline));
/*__INLINE__*/ void
norec_FreeList (norec_Log* k, long sz)
{
    /* Free appended overflow entries first */
    norec_AVPair* e = k->end;
    if (e != NULL) {
        while (e->Ordinal >= sz) {
            norec_AVPair* tmp = e;
            e = e->Prev;
            free(tmp);
        }
    }

    /* Free continguous beginning */
    free(k->List);
}

norec___INLINE__ norec_AVPair*
norec_ExtendList (norec_AVPair* tail)
{
    norec_AVPair* e = (norec_AVPair*)malloc(sizeof(*e));
    assert(e);
    memset(e, 0, sizeof(*e));
    tail->Next = e;
    e->Prev    = tail;
    e->Next    = NULL;
    e->Ordinal = tail->Ordinal + 1;
    return e;
}

inline intptr_t norec_MaskWord(intptr_t old, intptr_t val, intptr_t mask) {
	if(mask == ~0x0) {
		return val;
	}
	return (old & ~mask) | (val & mask);
}

norec___INLINE__ void
norec_WriteBackForward (norec_Log* k)
{
    norec_AVPair* e;
    norec_AVPair* End = k->put;
    for (e = k->List; e != End; e = e->Next) {
        *(e->Addr) = norec_MaskWord(*(e->Addr), e->Valu, e->Mask);
    }
}

void
norec_TxOnce ()
{
    norec_LOCK = (norec_aligned_type_t*) malloc(sizeof(norec_aligned_type_t));
    norec_LOCK->value = 0;

    pthread_key_create(&norec_global_key_self, NULL); /* CCM: do before we register handler */

}


void
norec_TxShutdown ()
{
    /*printf("NOREC system shutdown:\n"
           "  Starts=%li Aborts=%li\n"
           "  Overflows: R=%li W=%li L=%li\n"
           , StartTally, AbortTally,
           ReadOverflowTally, WriteOverflowTally, LocalOverflowTally);*/

    pthread_key_delete(norec_global_key_self);

    norec_MEMBARSTLD();
}


norec_Thread*
norec_TxNewThread ()
{
    norec_Thread* t = (norec_Thread*)malloc(sizeof(norec_Thread));
    assert(t);

    return t;
}


void
norec_TxFreeThread (norec_Thread* t)
{
    norec_AtomicAdd((volatile intptr_t*)((void*)(&norec_ReadOverflowTally)),  t->rdSet.ovf);

    long wrSetOvf = 0;
    norec_Log* wr;
    wr = &t->wrSet;
    {
        wrSetOvf += wr->ovf;
    }
    norec_AtomicAdd((volatile intptr_t*)((void*)(&norec_WriteOverflowTally)), wrSetOvf);

    norec_AtomicAdd((volatile intptr_t*)((void*)(&norec_StartTally)),         t->Starts);
    norec_AtomicAdd((volatile intptr_t*)((void*)(&norec_AbortTally)),         t->Aborts);

    norec_FreeList(&(t->rdSet),     NOREC_INIT_RDSET_NUM_ENTRY);
    norec_FreeList(&(t->wrSet),     NOREC_INIT_WRSET_NUM_ENTRY);

    free(t);
}

void
norec_TxInitThread (norec_Thread* t, long id)
{
    /* CCM: so we can access NOREC's thread metadata in signal handlers */
    pthread_setspecific(norec_global_key_self, (void*)t);

    memset(t, 0, sizeof(*t));     /* Default value for most members */

    t->UniqID = id;
    t->rng = id + 1;
    t->xorrng[0] = t->rng;

    t->wrSet.List = norec_MakeList(NOREC_INIT_WRSET_NUM_ENTRY, t);
    t->wrSet.put = t->wrSet.List;

    t->rdSet.List = norec_MakeList(NOREC_INIT_RDSET_NUM_ENTRY, t);
    t->rdSet.put = t->rdSet.List;


}

norec___INLINE__ void
norec_txReset (norec_Thread* Self)
{
    Self->wrSet.put = Self->wrSet.List;
    Self->wrSet.tail = NULL;

    Self->wrSet.BloomFilter = 0;
    Self->rdSet.put = Self->rdSet.List;
    Self->rdSet.tail = NULL;

    Self->using_hash_map = 1;
    if (Self->using_hash_map) {
    	memset(Self->norec_hash_write_set_markers, 0, sizeof(uint8_t) * NOREC_HASH_WRITE_SET_SIZE);
    }
}

norec___INLINE__ void
norec_txCommitReset (norec_Thread* Self)
{
    norec_txReset(Self);
    Self->Retries = 0;
}

// returns -1 if not coherent
norec___INLINE__ long
norec_ReadSetCoherent (norec_Thread* Self)
{
    long time;
    while (1) {
        norec_MEMBARSTLD();
        time = norec_LOCK->value;
        if ((time & 1) != 0) {
            continue;
        }

        norec_Log* const rd = &Self->rdSet;
        norec_AVPair* const EndOfList = rd->put;
        norec_AVPair* e;

        for (e = rd->List; e != EndOfList; e = e->Next) {
            if (e->Valu != norec_LDNF(e->Addr)) {
                return -1;
            }
        }

        if (norec_LOCK->value == time)
            break;
    }
    return time;
}


norec___INLINE__ void
norec_backoff (norec_Thread* Self, long attempt)
{
    unsigned long long stall = norec_TSRandom(Self) & 0xF;
    stall += attempt >> 2;
    stall *= 10;
    /* CCM: timer function may misbehave */
    volatile typeof(stall) i = 0;
    while (i++ < stall) {
        norec_PAUSE();
    }
}


norec___INLINE__ long
norec_TryFastUpdate (norec_Thread* Self)
{
    norec_Log* const wr = &Self->wrSet;
    long ctr;

    while (norec_CAS(&(norec_LOCK->value), Self->snapshot, Self->snapshot + 1) != Self->snapshot) {
        long newSnap = norec_ReadSetCoherent(Self);
        if (newSnap == -1) {
            return 0; //TxAbort(Self);
        }
        Self->snapshot = newSnap;
    }

    {
        norec_WriteBackForward(wr); /* write-back the deferred stores */
    }
    norec_MEMBARSTST(); /* Ensure the above stores are visible  */
    norec_LOCK->value = Self->snapshot + 2;
    norec_MEMBARSTLD();

    return 1; /* success */
}

void
norec_TxAbort (norec_Thread* Self)
{
    Self->Retries++;
    Self->Aborts++;

    if (Self->Retries > 3) { /* TUNABLE */
        norec_backoff(Self, Self->Retries);
    }

    norec_txReset(Self);
    norec_MEMBARLDLD();
    Self->Starts++;
    do {
        Self->snapshot = norec_LOCK->value;
    } while ((Self->snapshot & 1) != 0);

    norec_SIGLONGJMP(*Self->envPtr, 1);
    norec_ASSERT(0);
}

inline unsigned norec_map_address_to_hash_set_idx(intptr_t* address) {
	return (((uintptr_t)address) >> LOCK_EXTENT) & NOREC_HASH_WRITE_SET_MASK;
}

void
norec_TxStore_inner (norec_Thread* Self, volatile intptr_t* addr, intptr_t valu, intptr_t mask)
{
    norec_Log* k = &Self->wrSet;

    k->BloomFilter |= norec_FILTERBITS(addr);

    norec_AVPair* e = k->put;
    if (e == NULL) {
    	k->ovf++;
    	e = norec_ExtendList(k->tail);
    	k->end = e;
    }
    k->tail    = e;
    k->put     = e->Next;
    e->Addr    = addr;
    e->Valu    = valu;
    e->Mask    = mask;

    if (Self->using_hash_map) {
    	unsigned hash_write_set_idx = norec_map_address_to_hash_set_idx(addr);
    	if(Self->norec_hash_write_set_markers[hash_write_set_idx]) {
    		norec_AVPair* first_log_entry = Self->norec_hash_write_set[hash_write_set_idx];
    		e->NextHM = first_log_entry;
    		Self->norec_hash_write_set[hash_write_set_idx] = e;
    	} else {
    		Self->norec_hash_write_set_markers[hash_write_set_idx] = 1;
    		Self->norec_hash_write_set[hash_write_set_idx] = e;
    		e->NextHM = NULL;
    	}
    }
}

inline void norec_TxStore (norec_Thread* Self, volatile intptr_t* addr, intptr_t valu) {
    norec_Log* k = &Self->wrSet;

    k->BloomFilter |= norec_FILTERBITS(addr);

    norec_AVPair* e = k->put;
    if (e == NULL) {
    	k->ovf++;
    	e = norec_ExtendList(k->tail);
    	k->end = e;
    }
    k->tail    = e;
    k->put     = e->Next;
    e->Addr    = addr;
    e->Valu    = valu;
    e->Mask    = (intptr_t)~0x0;
}

inline void norec_TxStore_U8 (norec_Thread* thread, uint8_t* addr, uint8_t value) {
	unsigned pos = ((uintptr_t)addr & NOREC_MASK_64) >> NOREC_SHIFT_8;
	norec_read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b8[pos] = ~0;
	norec_read_write_conv_t val;
	val.b8[pos] = value;
	norec_TxStore_inner(thread, (intptr_t*)(((uintptr_t)addr) & ~NOREC_MASK_64), (intptr_t)val.b64[0], (intptr_t)mask.b64[0]);
}

inline void norec_TxStore_U16 (norec_Thread* thread, uint16_t* addr, uint16_t value) {
	unsigned pos = ((uintptr_t)addr & NOREC_MASK_64) >> NOREC_SHIFT_16;
	norec_read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b16[pos] = ~0;
	norec_read_write_conv_t val;
	val.b16[pos] = value;
	norec_TxStore_inner(thread, (intptr_t*)(((uintptr_t)addr) & ~NOREC_MASK_64), (intptr_t)val.b64[0], (intptr_t)mask.b64[0]);
}

inline void norec_TxStore_U32 (norec_Thread* thread, uint32_t* addr, uint32_t value) {
	unsigned pos = ((uintptr_t)addr & NOREC_MASK_64) >> NOREC_SHIFT_32;
	norec_read_write_conv_t mask;
	mask.b64[0] = 0;
	mask.b32[pos] = ~0;
	norec_read_write_conv_t val;
	val.b32[pos] = value;
	norec_TxStore_inner(thread, (intptr_t*)(((uintptr_t)addr) & ~NOREC_MASK_64), (intptr_t)val.b64[0], (intptr_t)mask.b64[0]);
}

inline void norec_TxStore_U64 (norec_Thread* thread, uint64_t* addr, uint64_t value) {
	norec_TxStore_inner(thread, (intptr_t*)addr, (intptr_t)value, (intptr_t)~0x0);
}

intptr_t
norec_TxLoad_inner (norec_Thread* Self, volatile intptr_t* Addr)
{
    intptr_t Valu;

    intptr_t msk = norec_FILTERBITS(Addr);
    if ((Self->wrSet.BloomFilter & msk) == msk) {

    	if (Self->using_hash_map == 0) {
    		unsigned long counter = 0;
    		norec_Log* wr = &Self->wrSet;
    		norec_AVPair* e;
    		unsigned char found = 0;
    		for (e = wr->tail; e != NULL; e = e->Prev, counter++) {
    			if (e->Addr == Addr) {
    				found = 1;
    				break;
    			}
    		}
    		if (counter > NOREC_WRITE_SET_THRESHOLD) {
    			norec_AVPair* End = wr->put;
    			for (e = wr->List; e != End; e = e->Next) {
        	    	unsigned hash_write_set_idx = norec_map_address_to_hash_set_idx(e->Addr);
        	    	if(Self->norec_hash_write_set_markers[hash_write_set_idx]) {
        	    		norec_AVPair* first_log_entry = Self->norec_hash_write_set[hash_write_set_idx];
        	    		e->NextHM = first_log_entry;
        	    	} else {
        	    		Self->norec_hash_write_set_markers[hash_write_set_idx] = 1;
        	    		Self->norec_hash_write_set[hash_write_set_idx] = e;
        	    		e->NextHM = NULL;
        	    	}
        		}
    			Self->using_hash_map = 1;
    		}
    		if (found) {
    			return norec_MaskWord(*(e->Addr), e->Valu, e->Mask);
    		}
    	} else {
    		unsigned hash_write_set_idx = norec_map_address_to_hash_set_idx(Addr);
    		norec_AVPair *first_log_entry, *log_entry;
    		first_log_entry = log_entry = Self->norec_hash_write_set[hash_write_set_idx];

    		if(Self->norec_hash_write_set_markers[hash_write_set_idx]) {
    			while(log_entry != NULL) {
    				if(log_entry->Addr == Addr) {
    					return norec_MaskWord(*(log_entry->Addr), log_entry->Valu, log_entry->Mask);
    				}
    				log_entry = log_entry->NextHM;
    			}
    		}
    	}
    }

    norec_MEMBARLDLD();
    Valu = norec_LDNF(Addr);
    while (norec_LOCK->value != Self->snapshot) {
        long newSnap = norec_ReadSetCoherent(Self);
        if (newSnap == -1) {
            norec_TxAbort(Self);
        }
        Self->snapshot = newSnap;
        norec_MEMBARLDLD();
        Valu = norec_LDNF(Addr);
    }

    norec_Log* k = &Self->rdSet;
    norec_AVPair* e = k->put;
    if (e == NULL) {
        k->ovf++;
        e = norec_ExtendList(k->tail);
        k->end = e;
    }
    k->tail    = e;
    k->put     = e->Next;
    e->Addr = Addr;
    e->Valu = Valu;

    return Valu;
}

// This is invoked in non-GCC instrumentation
inline intptr_t norec_TxLoad (norec_Thread* Self, volatile intptr_t* Addr) {
    intptr_t Valu;

    intptr_t msk = norec_FILTERBITS(Addr);
    if ((Self->wrSet.BloomFilter & msk) == msk) {
        norec_Log* wr = &Self->wrSet;
        norec_AVPair* e;
        for (e = wr->tail; e != NULL; e = e->Prev) {
            if (e->Addr == Addr) {
                return e->Valu;
            }
        }
    }

    norec_MEMBARLDLD();
    Valu = norec_LDNF(Addr);
    while (norec_LOCK->value != Self->snapshot) {
        long newSnap = norec_ReadSetCoherent(Self);
        if (newSnap == -1) {
            norec_TxAbort(Self);
        }
        Self->snapshot = newSnap;
        norec_MEMBARLDLD();
        Valu = norec_LDNF(Addr);
    }

    norec_Log* k = &Self->rdSet;
    norec_AVPair* e = k->put;
    if (e == NULL) {
        k->ovf++;
        e = norec_ExtendList(k->tail);
        k->end = e;
    }
    k->tail    = e;
    k->put     = e->Next;
    e->Addr = Addr;
    e->Valu = Valu;

    return Valu;
}

inline uint8_t norec_TxLoad_U8 (norec_Thread* thread, uint8_t* addr) {
	norec_read_write_conv_t value;
	value.b64[0] = (uint64_t)(norec_TxLoad_inner(thread, NOREC_WORD64_ADDRESS(addr)));
	return value.b8[NOREC_WORD64_8_POS(addr)];
}

inline uint16_t norec_TxLoad_U16 (norec_Thread* thread, uint16_t* addr) {
	norec_read_write_conv_t value;
	value.b64[0] = (uint64_t)(norec_TxLoad_inner(thread, NOREC_WORD64_ADDRESS(addr)));
	return value.b16[NOREC_WORD64_16_POS(addr)];
}

inline uint32_t norec_TxLoad_U32 (norec_Thread* thread, uint32_t* addr) {
	norec_read_write_conv_t value;
	value.b64[0] = (uint64_t)(norec_TxLoad_inner(thread, NOREC_WORD64_ADDRESS(addr)));
	return value.b32[NOREC_WORD64_32_POS(addr)];
}

inline uint64_t norec_TxLoad_U64 (norec_Thread* thread, uint64_t* addr) {
    return (uint64_t) norec_TxLoad_inner(thread, (intptr_t*)addr);
}

void
norec_TxStart (norec_Thread* Self, sigjmp_buf* buf)
{
    norec_txReset(Self);

    norec_MEMBARLDLD();

    Self->Starts++;
    Self->envPtr = buf;

    do {
        Self->snapshot = norec_LOCK->value;
    } while ((Self->snapshot & 1) != 0);
}

int
norec_TxCommit (norec_Thread* Self)
{
    /* Fast-path: Optional optimization for pure-readers */
    if (Self->wrSet.put == Self->wrSet.List)
    {
        norec_txCommitReset(Self);
        return 1;
    }

    if (norec_TryFastUpdate(Self)) {
        norec_txCommitReset(Self);
        return 1;
    }

    norec_TxAbort(Self);
    return 1; // why??
}

int
norec_TxCommitSTM (norec_Thread* Self)
{
    /* Fast-path: Optional optimization for pure-readers */
    if (Self->wrSet.put == Self->wrSet.List)
    {
        norec_txCommitReset(Self);
        return 1;
    }

    if (norec_TryFastUpdate(Self)) {
        norec_txCommitReset(Self);
        return 1;
    }

    return 0;
}

long norec_TxValidate (norec_Thread* Self) {
    if (Self->wrSet.put == Self->wrSet.List) {
        return -1;
    } else {
        long local_global_clock = norec_LOCK->value;

        while (1) {
            norec_Log* const rd = &Self->rdSet;
            norec_AVPair* const EndOfList = rd->put;
            norec_AVPair* e;

            for (e = rd->List; e != EndOfList; e = e->Next) {
                if (e->Valu != norec_LDNF(e->Addr)) {
                    norec_TxAbort(Self);
                }
            }

            long tmp = norec_LOCK->value;
            if (local_global_clock == tmp) {
                return local_global_clock;
            } else {
                local_global_clock = tmp;
            }
        }
        return local_global_clock;
    }
}


long norec_TxFinalize (norec_Thread* Self, long clock) {
    if (Self->wrSet.put == Self->wrSet.List) {
        norec_txCommitReset(Self);
        return 0;
    }

    if (norec_LOCK->value != clock) {
        return 1;
    }

    norec_Log* const wr = &Self->wrSet;
    norec_WriteBackForward(wr); /* write-back the deferred stores */
    norec_LOCK->value = norec_LOCK->value + 2;

    return 0;
}

void norec_TxResetAfterFinalize (norec_Thread* Self) {
    norec_txCommitReset(Self);
}
