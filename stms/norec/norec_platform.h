/* =============================================================================
 *
 * platform.h
 *
 * Platform-specific bindings
 *
 * =============================================================================
 */


#ifndef norec_PLATFORM_H
#define norec_PLATFORM_H 1


#if defined(SPARC) || defined(__sparc__)
#  include "norec_platform_sparc.h"
#else /* !SPARC (i.e., x86) */
#  include "norec_platform_x86.h"
#endif


#define norec_CAS(m,c,s)  cas((intptr_t)(s),(intptr_t)(c),(intptr_t*)(m))

typedef unsigned long long norec_TL2_TIMER_T;


#endif /* PLATFORM_H */


/* =============================================================================
 *
 * End of platform.h
 *
 * =============================================================================
 */
