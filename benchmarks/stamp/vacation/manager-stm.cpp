/* =============================================================================
 *
 * manager.c
 * -- Travel reservation resource manager
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
#include "pair-stm.h"
#include "manager-stm.h"
#include "customer-stm.h"
#include "reservation-stm.h"
#include "tm.h"
#include "types.h"

using namespace pair_stm;
using namespace customer_stm;
using namespace reservation_stm;

using namespace manager_stm;

/* =============================================================================
 * DECLARATION OF TM_CALLABLE FUNCTIONS
 * =============================================================================
 */
namespace manager_stm {

TM_CALLABLE
static long 
queryNumFree (TM_ARGDECL  MAP_T* tablePtr, long id);

TM_CALLABLE
static long 
queryPrice (TM_ARGDECL  MAP_T* tablePtr, long id);

TM_CALLABLE
static bool_t 
reserve (TM_ARGDECL MAP_T* tablePtr, MAP_T* customerTablePtr, long customerId, long id, reservation_type_t type);

TM_CALLABLE
bool_t
addReservation (TM_ARGDECL  MAP_T* tablePtr, long id, long num, long price);

}

bool_t
manager_stm::addReservation (TM_ARGDECL  MAP_T* tablePtr, long id, long num, long price)
{
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)rbtree_stm::TMrbtree_get(TM_ARG tablePtr, id);
    if (reservationPtr == NULL) {
        /* Create new reservation */
        if (num < 1 || price < 0) {
            return FALSE;
        }
        reservationPtr = RESERVATION_ALLOC(id, num, price);
        assert(reservationPtr != NULL);
        rbtree_stm::TMrbtree_insert(TM_ARG tablePtr, id, (int)reservationPtr);
    } else {
        /* Update existing reservation */
        if (!RESERVATION_ADD_TO_TOTAL(reservationPtr, num)) {
            return FALSE;
        }
        if ((long)SLOW_PATH_SHARED_READ(reservationPtr->numTotal) == 0) {
            bool_t status = rbtree_stm::TMrbtree_delete(TM_ARG tablePtr, id);
            if (status == FALSE) {
                SLOW_PATH_RESTART();
            }
            RESERVATION_FREE(reservationPtr);
        } else {
            RESERVATION_UPDATE_PRICE(reservationPtr, price);
        }
    }

    return TRUE;
}

/* =============================================================================
 * manager_addCar
 * -- Add cars to a city
 * -- Adding to an existing car overwrite the price if 'price' >= 0
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_addCar (TM_ARGDECL
                manager_t* managerPtr, long carId, long numCars, long price)
{
    return addReservation(TM_ARG  managerPtr->carTablePtr, carId, numCars, price);
}


bool_t
manager_stm::manager_deleteCar (TM_ARGDECL  manager_t* managerPtr, long carId, long numCar)
{
    /* -1 keeps old price */
    return addReservation(TM_ARG  managerPtr->carTablePtr, carId, -numCar, -1);
}


bool_t
manager_stm::manager_addRoom (TM_ARGDECL
                 manager_t* managerPtr, long roomId, long numRoom, long price)
{
    return addReservation(TM_ARG  managerPtr->roomTablePtr, roomId, numRoom, price);
}

/* =============================================================================
 * manager_deleteRoom
 * -- Delete rooms from a city
 * -- Decreases available room count (those not allocated to a customer)
 * -- Fails if would make available room count negative
 * -- If decresed to 0, deletes entire entry
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_deleteRoom (TM_ARGDECL  manager_t* managerPtr, long roomId, long numRoom)
{
    /* -1 keeps old price */
    return addReservation(TM_ARG  managerPtr->roomTablePtr, roomId, -numRoom, -1);
}


/* =============================================================================
 * manager_addFlight
 * -- Add seats to a flight
 * -- Adding to an existing flight overwrite the price if 'price' >= 0
 * -- Returns TRUE on success, FALSE on failure
 * =============================================================================
 */
bool_t
manager_stm::manager_addFlight (TM_ARGDECL
                   manager_t* managerPtr, long flightId, long numSeat, long price)
{
    return addReservation(TM_ARG
                          managerPtr->flightTablePtr, flightId, numSeat, price);
}

bool_t
manager_stm::manager_deleteFlight (TM_ARGDECL  manager_t* managerPtr, long flightId)
{
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)rbtree_stm::TMrbtree_get(TM_ARG managerPtr->flightTablePtr, flightId);
    if (reservationPtr == NULL) {
        return FALSE;
    }

    if ((long)SLOW_PATH_SHARED_READ(reservationPtr->numUsed) > 0) {
        return FALSE; /* somebody has a reservation */
    }

    return addReservation(TM_ARG
                          managerPtr->flightTablePtr,
                          flightId,
                          -1*(long)SLOW_PATH_SHARED_READ(reservationPtr->numTotal),
                          -1 /* -1 keeps old price */);
}


/* =============================================================================
 * manager_addCustomer
 * -- If customer already exists, returns failure
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_addCustomer (TM_ARGDECL  manager_t* managerPtr, long customerId)
{
    customer_t* customerPtr;
    bool_t status;

    if (rbtree_stm::TMrbtree_contains(TM_ARG managerPtr->customerTablePtr, customerId)) {
        return FALSE;
    }

    customerPtr = CUSTOMER_ALLOC(customerId);
    assert(customerPtr != NULL);
    status = rbtree_stm::TMrbtree_insert(TM_ARG managerPtr->customerTablePtr, customerId, (int)customerPtr);
    if (status == FALSE) {
        SLOW_PATH_RESTART();
    }

    return TRUE;
}


/* =============================================================================
 * manager_deleteCustomer
 * -- Delete this customer and associated reservations
 * -- If customer does not exist, returns success
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_deleteCustomer (TM_ARGDECL  manager_t* managerPtr, long customerId)
{
    customer_t* customerPtr;
    MAP_T* reservationTables[NUM_RESERVATION_TYPE];
    list_t* reservationInfoListPtr;
    list_iter_t it;
    bool_t status;

    customerPtr = (customer_t*)rbtree_stm::TMrbtree_get(TM_ARG managerPtr->customerTablePtr, customerId);
    if (customerPtr == NULL) {
        return FALSE;
    }

    reservationTables[RESERVATION_CAR] = managerPtr->carTablePtr;
    reservationTables[RESERVATION_ROOM] = managerPtr->roomTablePtr;
    reservationTables[RESERVATION_FLIGHT] = managerPtr->flightTablePtr;

    /* Cancel this customer's reservations */
    reservationInfoListPtr = customerPtr->reservationInfoListPtr;
    list_stm::TMlist_iter_reset(TM_ARG &it, reservationInfoListPtr);
    while (list_stm::TMlist_iter_hasNext(TM_ARG &it, reservationInfoListPtr)) {
        reservation_info_t* reservationInfoPtr;
        reservation_t* reservationPtr;
        reservationInfoPtr =
            (reservation_info_t*)list_stm::TMlist_iter_next(TM_ARG &it, reservationInfoListPtr);
        reservationPtr =
            (reservation_t*)rbtree_stm::TMrbtree_get(TM_ARG reservationTables[reservationInfoPtr->type],
                                     reservationInfoPtr->id);
        if (reservationPtr == NULL) {
            SLOW_PATH_RESTART();
        }
        status = RESERVATION_CANCEL(reservationPtr);
        if (status == FALSE) {
            SLOW_PATH_RESTART();
        }
        RESERVATION_INFO_FREE(reservationInfoPtr);
    }

    status = rbtree_stm::TMrbtree_delete(TM_ARG managerPtr->customerTablePtr, customerId);
    if (status == FALSE) {
        SLOW_PATH_RESTART();
    }
    CUSTOMER_FREE(customerPtr);

    return TRUE;
}


/* =============================================================================
 * QUERY INTERFACE
 * =============================================================================
 */


/* =============================================================================
 * queryNumFree
 * -- Return numFree of a reservation, -1 if failure
 * =============================================================================
 */
static long
manager_stm::queryNumFree (TM_ARGDECL  MAP_T* tablePtr, long id)
{
    long numFree = -1;
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)rbtree_stm::TMrbtree_get(TM_ARG tablePtr, id);
    if (reservationPtr != NULL) {
        numFree = (long)SLOW_PATH_SHARED_READ(reservationPtr->numFree);
    }

    return numFree;
}


/* =============================================================================
 * queryPrice
 * -- Return price of a reservation, -1 if failure
 * =============================================================================
 */
static long
manager_stm::queryPrice (TM_ARGDECL  MAP_T* tablePtr, long id)
{
    long price = -1;
    reservation_t* reservationPtr;

    reservationPtr = (reservation_t*)rbtree_stm::TMrbtree_get(TM_ARG tablePtr, id);
    if (reservationPtr != NULL) {
        price = (long)SLOW_PATH_SHARED_READ(reservationPtr->price);
    }

    return price;
}


/* =============================================================================
 * manager_queryCar
 * -- Return the number of empty seats on a car
 * -- Returns -1 if the car does not exist
 * =============================================================================
 */
long
manager_stm::manager_queryCar (TM_ARGDECL  manager_t* managerPtr, long carId)
{
    return queryNumFree(TM_ARG  managerPtr->carTablePtr, carId);
}


/* =============================================================================
 * manager_queryCarPrice
 * -- Return the price of the car
 * -- Returns -1 if the car does not exist
 * =============================================================================
 */
long
manager_stm::manager_queryCarPrice (TM_ARGDECL  manager_t* managerPtr, long carId)
{
    return queryPrice(TM_ARG  managerPtr->carTablePtr, carId);
}


/* =============================================================================
 * manager_queryRoom
 * -- Return the number of empty seats on a room
 * -- Returns -1 if the room does not exist
 * =============================================================================
 */
long
manager_stm::manager_queryRoom (TM_ARGDECL  manager_t* managerPtr, long roomId)
{
    return queryNumFree(TM_ARG  managerPtr->roomTablePtr, roomId);
}


/* =============================================================================
 * manager_queryRoomPrice
 * -- Return the price of the room
 * -- Returns -1 if the room does not exist
 * =============================================================================
 */
long
manager_stm::manager_queryRoomPrice (TM_ARGDECL  manager_t* managerPtr, long roomId)
{
    return queryPrice(TM_ARG  managerPtr->roomTablePtr, roomId);
}


/* =============================================================================
 * manager_queryFlight
 * -- Return the number of empty seats on a flight
 * -- Returns -1 if the flight does not exist
 * =============================================================================
 */
long
manager_stm::manager_queryFlight (TM_ARGDECL  manager_t* managerPtr, long flightId)
{
    return queryNumFree(TM_ARG  managerPtr->flightTablePtr, flightId);
}


/* =============================================================================
 * manager_queryFlightPrice
 * -- Return the price of the flight
 * -- Returns -1 if the flight does not exist
 * =============================================================================
 */
long
manager_stm::manager_queryFlightPrice (TM_ARGDECL  manager_t* managerPtr, long flightId)
{
    return queryPrice(TM_ARG  managerPtr->flightTablePtr, flightId);
}


/* =============================================================================
 * manager_queryCustomerBill
 * -- Return the total price of all reservations held for a customer
 * -- Returns -1 if the customer does not exist
 * =============================================================================
 */
long
manager_stm::manager_queryCustomerBill (TM_ARGDECL  manager_t* managerPtr, long customerId)
{
    long bill = -1;
    customer_t* customerPtr;

    customerPtr = (customer_t*)rbtree_stm::TMrbtree_get(TM_ARG managerPtr->customerTablePtr, customerId);

    if (customerPtr != NULL) {
        bill = CUSTOMER_GET_BILL(customerPtr);
    }

    return bill;
}


/* =============================================================================
 * RESERVATION INTERFACE
 * =============================================================================
 */


/* =============================================================================
 * reserve
 * -- Customer is not allowed to reserve same (type, id) multiple times
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
static bool_t
manager_stm::reserve (TM_ARGDECL
         MAP_T* tablePtr, MAP_T* customerTablePtr,
         long customerId, long id, reservation_type_t type)
{
    customer_t* customerPtr;
    reservation_t* reservationPtr;

    customerPtr = (customer_t*)rbtree_stm::TMrbtree_get(TM_ARG customerTablePtr, customerId);
    if (customerPtr == NULL) {
        return FALSE;
    }

    reservationPtr = (reservation_t*)rbtree_stm::TMrbtree_get(TM_ARG tablePtr, id);
    if (reservationPtr == NULL) {
        return FALSE;
    }

    if (!RESERVATION_MAKE(reservationPtr)) {
        return FALSE;
    }

    if (!CUSTOMER_ADD_RESERVATION_INFO(
            customerPtr,
            type,
            id,
            (long)SLOW_PATH_SHARED_READ(reservationPtr->price)))
    {
        /* Undo previous successful reservation */
        bool_t status = RESERVATION_CANCEL(reservationPtr);
        if (status == FALSE) {
            SLOW_PATH_RESTART();
        }
        return FALSE;
    }

    return TRUE;
}


/* =============================================================================
 * manager_reserveCar
 * -- Returns failure if the car or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_reserveCar (TM_ARGDECL  manager_t* managerPtr, long customerId, long carId)
{
    return reserve(TM_ARG
                   managerPtr->carTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   carId,
                   RESERVATION_CAR);
}


/* =============================================================================
 * manager_reserveRoom
 * -- Returns failure if the room or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_reserveRoom (TM_ARGDECL  manager_t* managerPtr, long customerId, long roomId)
{
    return reserve(TM_ARG
                   managerPtr->roomTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   roomId,
                   RESERVATION_ROOM);
}


/* =============================================================================
 * manager_reserveFlight
 * -- Returns failure if the flight or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_reserveFlight (TM_ARGDECL
                       manager_t* managerPtr, long customerId, long flightId)
{
    return reserve(TM_ARG
                   managerPtr->flightTablePtr,
                   managerPtr->customerTablePtr,
                   customerId,
                   flightId,
                   RESERVATION_FLIGHT);
}


/* =============================================================================
 * cancel
 * -- Customer is not allowed to cancel multiple times
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
static bool_t
cancel (TM_ARGDECL
        MAP_T* tablePtr, MAP_T* customerTablePtr,
        long customerId, long id, reservation_type_t type)
{
    customer_t* customerPtr;
    reservation_t* reservationPtr;

    customerPtr = (customer_t*)rbtree_stm::TMrbtree_get(TM_ARG customerTablePtr, customerId);
    if (customerPtr == NULL) {
        return FALSE;
    }

    reservationPtr = (reservation_t*)rbtree_stm::TMrbtree_get(TM_ARG tablePtr, id);
    if (reservationPtr == NULL) {
        return FALSE;
    }

    if (!RESERVATION_CANCEL(reservationPtr)) {
        return FALSE;
    }

    if (!CUSTOMER_REMOVE_RESERVATION_INFO(customerPtr, type, id)) {
        /* Undo previous successful cancellation */
        bool_t status = RESERVATION_MAKE(reservationPtr);
        if (status == FALSE) {
            SLOW_PATH_RESTART();
        }
        return FALSE;
    }

    return TRUE;
}


/* =============================================================================
 * manager_cancelCar
 * -- Returns failure if the car, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_cancelCar (TM_ARGDECL  manager_t* managerPtr, long customerId, long carId)
{
    return cancel(TM_ARG
                  managerPtr->carTablePtr,
                  managerPtr->customerTablePtr,
                  customerId,
                  carId,
                  RESERVATION_CAR);
}


/* =============================================================================
 * manager_cancelRoom
 * -- Returns failure if the room, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_cancelRoom (TM_ARGDECL  manager_t* managerPtr, long customerId, long roomId)
{
    return cancel(TM_ARG
                  managerPtr->roomTablePtr,
                  managerPtr->customerTablePtr,
                  customerId,
                  roomId,
                  RESERVATION_ROOM);
}



/* =============================================================================
 * manager_cancelFlight
 * -- Returns failure if the flight, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_stm::manager_cancelFlight (TM_ARGDECL
                      manager_t* managerPtr, long customerId, long flightId)
{
    return cancel(TM_ARG
                  managerPtr->flightTablePtr,
                  managerPtr->customerTablePtr,
                  customerId,
                  flightId,
                  RESERVATION_FLIGHT);
}
