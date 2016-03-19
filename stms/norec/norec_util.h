/* =============================================================================
 *
 * util.h
 *
 * Collection of useful utility routines
 *
 * =============================================================================
 */


#ifndef norec_UTIL_H
#define norec_UTIL_H


#include <assert.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


#define norec_DIM(A)                          (sizeof(A)/sizeof((A)[0]))
#define norec_UNS(a)                          ((uintptr_t)(a))
#define norec_ASSERT(x)                       /* assert(x) */
#define norec_CTASSERT(x)                     ({ int a[1-(2*!(x))]; a[0] = 0;})


/*
 * Shorthand for type conversion routines
 */

#define norec_IP2D(v)							norec_intp2double(v)
#define norec_IP2F(v)                         norec_intp2float(v)
#define norec_D2IP(v)                         norec_double2intp(v)
#define norec_F2IP(v)                         norec_float2intp(v)

#define norec_IPP2FP(v)                       norec_intpp2floatpp(v)
#define norec_DP2IPP(v)                       norec_doublep2intpp(v)
#define norec_FP2IPP(v)                       norec_floatp2intpp(v)

#define norec_IP2VP(v)                        norec_intp2voidp(v)
#define norec_VP2IP(v)                        norec_voidp2intp(v)


/* =============================================================================
 * intp2float
 * =============================================================================
 */
static __inline__ float
norec_intp2float (intptr_t val)
{
#ifdef __LP64__
    union {
        intptr_t i;
        float    f[2];
    } convert;
    convert.i = val;
    return convert.f[0];
#else
    union {
        intptr_t i;
        float    f;
    } convert;
    convert.i = val;
    return convert.f;
#endif
}

static __inline__ float
norec_intp2double (intptr_t val)
{
	union {
		intptr_t i;
		double    d;
	} convert;
	convert.i = val;
	return convert.d;
}

/* =============================================================================
 * float2intp
 * =============================================================================
 */
static __inline__ intptr_t
norec_float2intp (float val)
{
#ifdef __LP64__
    union {
        intptr_t i;
        float    f[2];
    } convert;
    convert.f[0] = val;
    return convert.i;
#else
    union {
        intptr_t i;
        float    f;
    } convert;
    convert.f = val;
    return convert.i;
#endif
}

static __inline__ intptr_t
norec_double2intp (double val)
{
	union {
		intptr_t i;
		double    d;
	} convert;
	convert.d = val;
	return convert.i;
}


/* =============================================================================
 * intpp2floatp
 * =============================================================================
 */
static __inline__ float*
norec_intpp2floatp (intptr_t* val)
{
    union {
        intptr_t* i;
        float*    f;
    } convert;
    convert.i = val;
    return convert.f;
}


/* =============================================================================
 * floatp2intpp
 * =============================================================================
 */
static __inline__ intptr_t*
norec_floatp2intpp (float* val)
{
    union {
        intptr_t* i;
        float*    f;
    } convert;
    convert.f = val;
    return convert.i;
}


static __inline__ intptr_t*
norec_doublep2intpp (double* val)
{
    union {
        intptr_t* i;
        double*    d;
    } convert;
    convert.d = val;
    return convert.i;
}

/* =============================================================================
 * intp2voidp
 * =============================================================================
 */
static __inline__ void*
norec_intp2voidp (intptr_t val)
{
    union {
        intptr_t i;
        void*    v;
    } convert;
    convert.i = val;
    return convert.v;
}


/* =============================================================================
 * voidp2intp
 * =============================================================================
 */
static __inline__ intptr_t
norec_voidp2intp (void* val)
{
    union {
        intptr_t i;
        void*    v;
    } convert;
    convert.v = val;
    return convert.i;
}


/* =============================================================================
 * CompileTimeAsserts
 *
 * Establish critical invariants and fail at compile-time rather than run-time
 * =============================================================================
 */
static __inline__ void
norec_CompileTimeAsserts ()
{
#ifdef __LP64__
    norec_CTASSERT(sizeof(intptr_t) == sizeof(long));
    norec_CTASSERT(sizeof(long) == 8);
#else
    norec_CTASSERT(sizeof(intptr_t) == sizeof(long));
    norec_CTASSERT(sizeof(long) == 4);
#endif

    /*
     * For type conversions
     */
#ifdef __LP64__
    norec_CTASSERT(2*sizeof(float) == sizeof(intptr_t));
#else
    norec_CTASSERT(sizeof(float)   == sizeof(intptr_t));
#endif
    norec_CTASSERT(sizeof(float*)  == sizeof(intptr_t));
    norec_CTASSERT(sizeof(void*)   == sizeof(intptr_t));
}


#ifdef __cplusplus
}
#endif


#endif /* UTIL_H */
