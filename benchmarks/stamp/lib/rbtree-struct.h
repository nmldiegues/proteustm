#ifndef RBTREE_H_
#define RBTREE_H_ 1

#include <cstdint>


#if defined(MAP_USE_AVLTREE)
#  include "avltree.h"

#  define MAP_T                       jsw_avltree_t
#  define MAP_ALLOC(hash, cmp)        jsw_avlnew((cmp_f)cmp)
#  define MAP_FREE(map)               jsw_avldelete(map)
#  define MAP_CONTAINS(map, key) \
    ({ \
        bool_t success = FALSE; \
        pair_t searchPair; \
        searchPair.firstPtr = (void*)key; \
        if (jsw_avlfind(map, (void*)&searchPair) != NULL) { \
            success = TRUE; \
        } \
        success; \
     })
#  define MAP_FIND(map, key) \
    ({ \
        void* dataPtr = NULL; \
        pair_t searchPair; \
        searchPair.firstPtr = (void*)(key); \
        pair_t* pairPtr = (pair_t*)jsw_avlfind(map, (void*)&searchPair); \
        if (pairPtr != NULL) { \
            dataPtr = pairPtr->secondPtr; \
        } \
        dataPtr; \
     })
#  define MAP_INSERT(map, key, data) \
    ({ \
        bool_t success = FALSE; \
        pair_t* insertPtr = pair_alloc((void*)(key), (void*)data); \
        if (insertPtr != NULL) { \
            if (jsw_avlinsert(map, (void*)insertPtr)) { \
                success = TRUE; \
            } \
        } \
        success; \
     })
#  define MAP_REMOVE(map, key) \
    ({ \
        bool_t success = FALSE; \
        pair_t searchPair; \
        searchPair.firstPtr = (void*)(key); \
        pair_t* pairPtr = (pair_t*)jsw_avlfind(map, (void*)&searchPair); \
        if (jsw_avlerase(map, (void*)&searchPair)) { \
            pair_free(pairPtr); \
            success = TRUE; \
        } \
        success; \
     })

#  define PMAP_ALLOC(hash, cmp)        Pjsw_avlnew((cmp_f)cmp)
#  define PMAP_FREE(map)               Pjsw_avldelete(map)
#  define PMAP_INSERT(map, key, data) \
    ({ \
        bool_t success = FALSE; \
        pair_t* insertPtr = PPAIR_ALLOC((void*)(key), (void*)data); \
        if (insertPtr != NULL) { \
            if (Pjsw_avlinsert(map, (void*)insertPtr)) { \
                success = TRUE; \
            } \
        } \
        success; \
     })
#  define PMAP_REMOVE(map, key) \
    ({ \
        bool_t success = FALSE; \
        pair_t searchPair; \
        searchPair.firstPtr = (void*)(key); \
        pair_t* pairPtr = (pair_t*)jsw_avlfind(map, (void*)&searchPair); \
        if (Pjsw_avlerase(map, (void*)&searchPair)) { \
            PPAIR_FREE(pairPtr); \
            success = TRUE; \
        } \
        success; \
     })
#else
#  define MAP_T                       rbtree_t
#endif

/*
#  define TMMAP_CONTAINS(map, key)    TMRBTREE_CONTAINS(map, (void*)(key))
#  define TMMAP_FIND(map, key)        TMRBTREE_GET(map, (void*)(key))
#  define TMMAP_INSERT(map, key, data) \
    TMRBTREE_INSERT(map, (void*)(key), (void*)(data))
#  define TMMAP_REMOVE(map, key)      TMRBTREE_DELETE(map, (void*)(key))
*/

#ifdef SET_USE_RBTREE
#  define SET_T                       rbtree_t
#endif

/*
#  define TMSET_CONTAINS(map, key)    TMRBTREE_CONTAINS(map, (void*)(key))
#  define TMSET_INSERT(map, key)      TMRBTREE_INSERT(map, (void*)(key), NULL)
#  define TMSET_REMOVE(map, key)      TMRBTREE_DELETE(map, (void*)(key))
*/

typedef struct node {
    intptr_t k;
    intptr_t v;
    struct node* p;
    struct node* l;
    struct node* r;
    intptr_t c;
} node_t;


typedef struct rbtree {
    node_t* root;
    long (*compare)(const void*, const void*);   /* returns {-1,0,1}, 0 -> equal */
} rbtree_t;


#endif
