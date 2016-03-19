/* =============================================================================
 *
 * customer.c
 * -- Representation of customer
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


#include <assert.h>
#include <stdlib.h>
#include "customer-htm.h"
#include "memory.h"
#include "tm.h"
#include "types.h"

using namespace customer_htm;

/* =============================================================================
 * compareReservationInfo
 * =============================================================================
 */
static long
compareReservationInfo (const void* aPtr, const void* bPtr)
{
    return reservation_info_compare((reservation_info_t*)aPtr,
                                    (reservation_info_t*)bPtr);
}


/* =============================================================================
 * customer_alloc
 * =============================================================================
 */
customer_t*
customer_htm::customer_alloc (TM_ARGDECL  long id)
{
    customer_t* customerPtr;

    customerPtr = (customer_t*)TM_MALLOC(sizeof(customer_t));
    assert(customerPtr != NULL);

    customerPtr->id = id;

    customerPtr->reservationInfoListPtr = list_htm::TMlist_alloc(TM_ARG &compareReservationInfo);
    assert(customerPtr->reservationInfoListPtr != NULL);

    return customerPtr;
}


customer_t*
customer_htm::customer_alloc_seq (long id)
{
    customer_t* customerPtr;

    customerPtr = (customer_t*)malloc(sizeof(customer_t));
    assert(customerPtr != NULL);

    customerPtr->id = id;

    customerPtr->reservationInfoListPtr = list_alloc(&compareReservationInfo);
    assert(customerPtr->reservationInfoListPtr != NULL);

    return customerPtr;
}


/* =============================================================================
 * customer_compare
 * -- Returns -1 if A < B, 0 if A = B, 1 if A > B
 * =============================================================================
 */
long
customer_htm::customer_compare (customer_t* aPtr, customer_t* bPtr)
{
    return (aPtr->id - bPtr->id);
}


/* =============================================================================
 * customer_free
 * =============================================================================
 */
void
customer_htm::customer_free (TM_ARGDECL  customer_t* customerPtr)
{
    list_t* reservationInfoListPtr =
        (list_t*)FAST_PATH_SHARED_READ(customerPtr->reservationInfoListPtr);
    list_htm::TMlist_free(TM_ARG reservationInfoListPtr);
    FAST_PATH_FREE(customerPtr);
}


/* =============================================================================
 * customer_addReservationInfo
 * -- Returns TRUE if success, else FALSE
 * =============================================================================
 */
bool_t
customer_htm::customer_addReservationInfo (TM_ARGDECL
                             customer_t* customerPtr,
                             reservation_type_t type, long id, long price)
{
    reservation_info_t* reservationInfoPtr;

    reservationInfoPtr = RESERVATION_INFO_ALLOC(type, id, price);
    assert(reservationInfoPtr != NULL);

    list_t* reservationInfoListPtr =
        (list_t*)FAST_PATH_SHARED_READ(customerPtr->reservationInfoListPtr);

    return list_htm::TMlist_insert(TM_ARG reservationInfoListPtr, (void*)reservationInfoPtr);
}


/* =============================================================================
 * customer_removeReservationInfo
 * -- Returns TRUE if success, else FALSE
 * =============================================================================
 */
bool_t
customer_htm::customer_removeReservationInfo (TM_ARGDECL
                                customer_t* customerPtr,
                                reservation_type_t type, long id)
{
    reservation_info_t findReservationInfo;

    findReservationInfo.type = type;
    findReservationInfo.id = id;
    /* price not used to compare reservation infos */

    list_t* reservationInfoListPtr =
        (list_t*)FAST_PATH_SHARED_READ(customerPtr->reservationInfoListPtr);

    reservation_info_t* reservationInfoPtr =
        (reservation_info_t*)list_htm::TMlist_find(TM_ARG reservationInfoListPtr,
                                         &findReservationInfo);

    if (reservationInfoPtr == NULL) {
        return FALSE;
    }

    bool_t status = list_htm::TMlist_remove(TM_ARG reservationInfoListPtr,
                                  (void*)&findReservationInfo);
    if (status == FALSE) {
        FAST_PATH_RESTART();
    }

    RESERVATION_INFO_FREE(reservationInfoPtr);

    return TRUE;
}


/* =============================================================================
 * customer_getBill
 * -- Returns total cost of reservations
 * =============================================================================
 */
long
customer_htm::customer_getBill (TM_ARGDECL  customer_t* customerPtr)
{
    long bill = 0;
    list_iter_t it;
    list_t* reservationInfoListPtr =
        (list_t*)FAST_PATH_SHARED_READ(customerPtr->reservationInfoListPtr);

    list_htm::TMlist_iter_reset(TM_ARG &it, reservationInfoListPtr);
    while (list_htm::TMlist_iter_hasNext(TM_ARG &it, reservationInfoListPtr)) {
        reservation_info_t* reservationInfoPtr =
            (reservation_info_t*)list_htm::TMlist_iter_next(TM_ARG &it, reservationInfoListPtr);
        bill += reservationInfoPtr->price;
    }

    return bill;
}


