/* =============================================================================
 *
 * list.h
 * -- Sorted singly linked list
 * -- Options: -DLIST_NO_DUPLICATES (default: allow duplicates)
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 *
 * ------------------------------------------------------------------------
 *
 * For the license of ssca2, please see ssca2/COPYRIGHT
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 *
 * ------------------------------------------------------------------------
 *
 * Unless otherwise noted, the following license applies to STAMP files:
 *
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#ifndef LIST_H
#define LIST_H 1

#include "tm.h"
#include "types.h"


typedef struct list_node {
    void* dataPtr;
    struct list_node* nextPtr;
} list_node_t;

typedef list_node_t* list_iter_t;

typedef struct list {
    list_node_t head;
    long (*compare)(const void*, const void*);   /* returns {-1,0,1}, 0 -> equal */
    long size;
} list_t;



#define PLIST_ALLOC(cmp)                Plist_alloc(cmp)
#define PLIST_FREE(list)                Plist_free(list)
#define PLIST_GETSIZE(list)             list_getSize(list)
#define PLIST_INSERT(list, data)        Plist_insert(list, data)
#define PLIST_REMOVE(list, data)        Plist_remove(list, data)
#define PLIST_CLEAR(list)               Plist_clear(list)


#define TMLIST_ITER_RESET(it, list)     TMlist_iter_reset(TM_ARG  it, list)
#define TMLIST_ITER_HASNEXT(it, list)   TMlist_iter_hasNext(TM_ARG  it, list)
#define TMLIST_ITER_NEXT(it, list)      TMlist_iter_next(TM_ARG  it, list)
#define TMLIST_ALLOC(cmp)               TMlist_alloc(TM_ARG  cmp)
#define TMLIST_FREE(list)               TMlist_free(TM_ARG  list)
#define TMLIST_GETSIZE(list)            TMlist_getSize(TM_ARG  list)
#define TMLIST_ISEMPTY(list)            TMlist_isEmpty(TM_ARG  list)
#define TMLIST_FIND(list, data)         TMlist_find(TM_ARG  list, data)
#define TMLIST_INSERT(list, data)       TMlist_insert(TM_ARG  list, data)
#define TMLIST_REMOVE(list, data)       TMlist_remove(TM_ARG  list, data)



#endif /* LIST_H */
