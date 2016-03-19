/* =============================================================================
 *
 * rbtree.h
 * -- Red-black balanced binary search tree
 *
 * =============================================================================
 *
 * Copyright (C) Sun Microsystems Inc., 2006.  All Rights Reserved.
 * Authors: Dave Dice, Nir Shavit, Ori Shalev.
 *
 * STM: Transactional Locking for Disjoint Access Parallelism
 *
 * Transactional Locking II,
 * Dave Dice, Ori Shalev, Nir Shavit
 * DISC 2006, Sept 2006, Stockholm, Sweden.
 *
 * =============================================================================
 *
 * Modified by Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
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


#ifndef RBTREE_STM_H
#define RBTREE_STM_H 1


#include <inttypes.h>
#include "tm.h"
#include "rbtree-struct.h"


namespace rbtree_stm {



/* =============================================================================
 * rbtree_verify
 * =============================================================================
 */
long
rbtree_verify (rbtree_t* s, long verbose);


/* =============================================================================
 * rbtree_alloc
 * =============================================================================
 */
rbtree_t*
rbtree_alloc (long (*compare)(const void*, const void*));


/* =============================================================================
 * TMrbtree_alloc
 * =============================================================================
 */
rbtree_t*
TMrbtree_alloc (TM_ARGDECL  long (*compare)(const void*, const void*));


/* =============================================================================
 * rbtree_free
 * =============================================================================
 */
void
rbtree_free (rbtree_t* r);


/* =============================================================================
 * TMrbtree_free
 * =============================================================================
 */
void
TMrbtree_free (TM_ARGDECL  rbtree_t* r);


/* =============================================================================
 * rbtree_insert
 * -- Returns TRUE on success
 * =============================================================================
 */
bool_t
rbtree_insert (rbtree_t* r, void* key, void* val);


/* =============================================================================
 * TMrbtree_insert
 * -- Returns TRUE on success
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMrbtree_insert (TM_ARGDECL  rbtree_t* r, void* key, void* val);


/* =============================================================================
 * rbtree_delete
 * =============================================================================
 */
bool_t
rbtree_delete (rbtree_t* r, void* key);


/* =============================================================================
 * TMrbtree_delete
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMrbtree_delete (TM_ARGDECL  rbtree_t* r, void* key);


/* =============================================================================
 * rbtree_update
 * -- Return FALSE if had to insert node first
 * =============================================================================
 */
bool_t
rbtree_update (rbtree_t* r, void* key, void* val);


/* =============================================================================
 * TMrbtree_update
 * -- Return FALSE if had to insert node first
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMrbtree_update (TM_ARGDECL  rbtree_t* r, void* key, void* val);


/* =============================================================================
 * rbtree_get
 * =============================================================================
 */
void*
rbtree_get (rbtree_t* r, void* key);


/* =============================================================================
 * TMrbtree_get
 * =============================================================================
 */
TM_CALLABLE
void*
TMrbtree_get (TM_ARGDECL  rbtree_t* r, void* key);


/* =============================================================================
 * rbtree_contains
 * =============================================================================
 */
bool_t
rbtree_contains (rbtree_t* r, void* key);


/* =============================================================================
 * TMrbtree_contains
 * =============================================================================
 */
TM_CALLABLE
bool_t
TMrbtree_contains (TM_ARGDECL  rbtree_t* r, void* key);

#if !defined(MAP_USE_AVLTREE)
#  define MAP_ALLOC(hash, cmp)        rbtree_alloc(cmp)
#  define MAP_FREE(map)               rbtree_free(map)

#  define MAP_CONTAINS(map, key)      rbtree_contains(map, (void*)(key))
#  define MAP_FIND(map, key)          rbtree_get(map, (void*)(key))
#  define MAP_INSERT(map, key, data) \
    rbtree_insert(map, (void*)(key), (void*)(data))
#  define MAP_REMOVE(map, key)        rbtree_delete(map, (void*)(key))
#endif

#  define SET_T                       rbtree_t
#  define SET_ALLOC(hash, cmp)        rbtree_alloc(cmp)
#  define SET_FREE(map)               rbtree_free(map)

#  define SET_CONTAINS(map, key)      rbtree_contains(map, (void*)(key))
#  define SET_INSERT(map, key)        rbtree_insert(map, (void*)(key), NULL)
#  define SET_REMOVE(map, key)        rbtree_delete(map, (void*)(key))

/*
#define TMRBTREE_ALLOC()          TMrbtree_alloc(TM_ARG_ALONE)
#define TMRBTREE_FREE(r)          TMrbtree_free(TM_ARG  r)
#define TMRBTREE_INSERT(r, k, v)  TMrbtree_insert(TM_ARG  r, (int)(k), (int)(v))
#define TMRBTREE_DELETE(r, k)     TMrbtree_delete(TM_ARG  r, (int)(k))
#define TMRBTREE_UPDATE(r, k, v)  TMrbtree_update(TM_ARG  r, (int)(k), (int)(v))
#define TMRBTREE_GET(r, k)        TMrbtree_get(TM_ARG  r, (int)(k))
#define TMRBTREE_CONTAINS(r, k)   TMrbtree_contains(TM_ARG  r, (int)(k))

#  define TMSET_CONTAINS(map, key)    TMRBTREE_CONTAINS(map, (void*)(key))
#  define TMSET_INSERT(map, key)      TMRBTREE_INSERT(map, (void*)(key), NULL)
#  define TMSET_REMOVE(map, key)      TMRBTREE_DELETE(map, (void*)(key))
*/

} // end namespace rbtree_stm


#endif /* RBTREE_H */



/* =============================================================================
 *
 * End of rbtree.h
 *
 * =============================================================================
 */
