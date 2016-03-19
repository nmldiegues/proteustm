// use this wrapper in order to more easily define macros needed by
// gc and/or some other libraries

#ifndef PTHREAD_WRAPPER_H_
#define PTHREAD_WRAPPER_H_ 1

#ifdef MM_GC_BOEHM

#define GC_THREADS
#define GC_REDIRECT_TO_LOCAL
#include "gc.h"

#endif

#include <pthread.h>

#endif
