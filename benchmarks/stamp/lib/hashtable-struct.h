#ifndef HASHTABLE_H
#define HASHTABLE_H 1


#include "list-struct.h"
#include "pair-struct.h"
#include "tm.h"
#include "types.h"


enum hashtable_config {
    HASHTABLE_DEFAULT_RESIZE_RATIO  = 3,
    HASHTABLE_DEFAULT_GROWTH_FACTOR = 3
};

typedef struct hashtable {
    list_t** buckets;
    long numBucket;
#ifdef HASHTABLE_SIZE_FIELD
    long size;
#endif
    ulong_t (*hash)(const void*);
    long (*comparePairs)(const pair_t*, const pair_t*);
    long resizeRatio;
    long growthFactor;
    /* comparePairs should return <0 if before, 0 if equal, >0 if after */
} hashtable_t;


typedef struct hashtable_iter {
    long bucket;
    list_iter_t it;
} hashtable_iter_t;



#endif /* HASHTABLE_H */
