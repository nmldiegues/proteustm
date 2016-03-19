/* =============================================================================
 *
 * norec_platform_sparc.h
 *
 * SPARC-specific bindings
 *
 * =============================================================================
 */


#ifndef norec_PLATFORM_SPARC_H
#define norec_PLATFORM_SPARC_H 1


#ifndef norec_PLATFORM_H
#  error include "norec_platform.h" for "norec_platform_sparc.h"
#endif


#include <stdint.h>
#include "norec_common.h"


/* =============================================================================
 * Compare-and-swap
 *
 * =============================================================================
 */
norec___INLINE__ intptr_t
norec_cas (intptr_t newVal, intptr_t oldVal, intptr_t* ptr)
{
    intptr_t prevVal;

    __asm__ __volatile__(
#ifdef __LP64__
        "casx [%2],%3,%1"
#else
        "cas [%2],%3,%1"
#endif
        : "=r"(prevVal)
        : "0"(newVal), "r"(ptr), "r"(oldVal)
        : "memory"
    );

    return prevVal;
}


/* =============================================================================
 * Memory Barriers
 * =============================================================================
 */
#define norec_MEMBARLDLD()   /* nothing */
#define norec_MEMBARSTST()   /* nothing */
#define norec_MEMBARSTLD()   __asm__ __volatile__ ("membar #StoreLoad" : : :"memory")


/* =============================================================================
 * Prefetching
 *
 * We use PREFETCHW in LD...CAS and LD...ST circumstances to force the $line
 * directly into M-state, avoiding RTS->RTO upgrade txns.
 * =============================================================================
 */
norec___INLINE__ void
norec_prefetchw (volatile void* x)
{
    __asm__ __volatile__ ("prefetch %0,2" :: "m" (x));
}


/* =============================================================================
 * Non-faulting load
 * =============================================================================
 */
norec___INLINE__ intptr_t
norec_LDNF (volatile intptr_t* a)
{
    intptr_t x;

    __asm__ __volatile__ (
#ifdef __LP64__
        "ldxa [%1]0x82, %0"
#else
        "ldswa [%1]0x82, %0" /* 0x82 = #ASI_PNF = Addr Space Primary Non-Fault */
#endif
        : "=&r"(x)
        : "r"(a)
        : "memory"
    );

    return x;
}


/* =============================================================================
 * MP-polite spinning
 * -- Ideally we would like to drop the priority of our CMT strand.
 * =============================================================================
 */
#define norec_PAUSE()  /* nothing */


/* =============================================================================
 * Timer functions
 * =============================================================================
 */
#define norec_TL2_TIMER_READ() gethrtime()


#endif /* PLATFORM_SPARC_H */


/* =============================================================================
 *
 * End of norec_platform_sparc.h
 *
 * =============================================================================
 */
