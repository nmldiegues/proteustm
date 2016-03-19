/* =============================================================================
 *
 * manager.h
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


#ifndef MANAGER_STM_H
#define MANAGER_STM_H 1


#include "manager-struct.h"
#include "tm.h"
#include "types.h"
#include "rbtree-stm.h"

using namespace rbtree_stm;

namespace manager_stm {

bool_t
manager_addCar (TM_ARGDECL
                manager_t* managerPtr, long carId, long numCar, long price);

bool_t
manager_deleteCar (TM_ARGDECL  manager_t* managerPtr, long carId, long numCar);

bool_t
manager_addRoom (TM_ARGDECL
                 manager_t* managerPtr, long roomId, long numRoom, long price);

bool_t
manager_deleteRoom (TM_ARGDECL  manager_t* managerPtr, long roomId, long numRoom);

bool_t
manager_addFlight (TM_ARGDECL
                   manager_t* managerPtr, long flightId, long numSeat, long price);

bool_t
manager_deleteFlight (TM_ARGDECL  manager_t* managerPtr, long flightId);

bool_t
manager_addCustomer (TM_ARGDECL  manager_t* managerPtr, long customerId);

bool_t
manager_deleteCustomer (TM_ARGDECL  manager_t* managerPtr, long customerId);

long
manager_queryCar (TM_ARGDECL  manager_t* managerPtr, long carId);

long
manager_queryCarPrice (TM_ARGDECL  manager_t* managerPtr, long carId);


long
manager_queryRoom (TM_ARGDECL  manager_t* managerPtr, long roomId);

long
manager_queryRoomPrice (TM_ARGDECL  manager_t* managerPtr, long roomId);

long
manager_queryFlight (TM_ARGDECL  manager_t* managerPtr, long flightId);

long
manager_queryFlightPrice (TM_ARGDECL  manager_t* managerPtr, long flightId);

long
manager_queryCustomerBill (TM_ARGDECL  manager_t* managerPtr, long customerId);

bool_t
manager_reserveCar (TM_ARGDECL
                    manager_t* managerPtr, long customerId, long carId);


/* =============================================================================
 * manager_reserveRoom
 * -- Returns failure if the room or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_reserveRoom (TM_ARGDECL
                     manager_t* managerPtr, long customerId, long roomId);


/* =============================================================================
 * manager_reserveFlight
 * -- Returns failure if the flight or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_reserveFlight (TM_ARGDECL
                       manager_t* managerPtr, long customerId, long flightId);


/* =============================================================================
 * manager_cancelCar
 * -- Returns failure if the car, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_cancelCar (TM_ARGDECL
                   manager_t* managerPtr, long customerId, long carId);


/* =============================================================================
 * manager_cancelRoom
 * -- Returns failure if the room, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_cancelRoom (TM_ARGDECL
                    manager_t* managerPtr, long customerId, long roomId);


/* =============================================================================
 * manager_cancelFlight
 * -- Returns failure if the flight, reservation, or customer does not exist
 * -- Returns TRUE on success, else FALSE
 * =============================================================================
 */
bool_t
manager_cancelFlight (TM_ARGDECL
                      manager_t* managerPtr, long customerId, long flightId);


#define MANAGER_ADD_CAR(mgr, id, num, price) \
    manager_addCar(TM_ARG  mgr, id, num, price)
#define MANAGER_DELETE_CAR(mgr, id, num) \
    manager_deleteCar(TM_ARG  mgr, id, num)
#define MANAGER_ADD_ROOM(mgr, id, num, price) \
    manager_addRoom(TM_ARG  mgr, id, num, price)
#define MANAGER_DELETE_ROOM(mgr, id, num) \
    manager_deleteRoom(TM_ARG  mgr, id, num)
#define MANAGER_ADD_FLIGHT(mgr, id, num, price) \
    manager_addFlight(TM_ARG  mgr, id, num, price)
#define MANAGER_DELETE_FLIGHT(mgr, id) \
    manager_deleteFlight(TM_ARG  mgr, id)
#define MANAGER_ADD_CUSTOMER(mgr, id) \
    manager_addCustomer(TM_ARG  mgr, id)
#define MANAGER_DELETE_CUSTOMER(mgr, id) \
    manager_deleteCustomer(TM_ARG  mgr, id)
#define MANAGER_QUERY_CAR(mgr, id) \
    manager_queryCar(TM_ARG  mgr, id)
#define MANAGER_QUERY_CAR_PRICE(mgr, id) \
    manager_queryCarPrice(TM_ARG  mgr, id)
#define MANAGER_QUERY_ROOM(mgr, id) \
    manager_queryRoom(TM_ARG  mgr, id)
#define MANAGER_QUERY_ROOM_PRICE(mgr, id) \
    manager_queryRoomPrice(TM_ARG  mgr, id)
#define MANAGER_QUERY_FLIGHT(mgr, id) \
    manager_queryFlight(TM_ARG  mgr, id)
#define MANAGER_QUERY_FLIGHT_PRICE(mgr, id) \
    manager_queryFlightPrice(TM_ARG  mgr, id)
#define MANAGER_QUERY_CUSTOMER_BILL(mgr, id) \
    manager_queryCustomerBill(TM_ARG  mgr, id)
#define MANAGER_RESERVE_CAR(mgr, cust, id) \
    manager_reserveCar(TM_ARG  mgr, cust, id)
#define MANAGER_RESERVE_ROOM(mgr, cust, id) \
    manager_reserveRoom(TM_ARG  mgr, cust, id)
#define MANAGER_RESERVE_FLIGHT(mgr, cust, id) \
    manager_reserveFlight(TM_ARG  mgr, cust, id)
#define MANAGER_CANCEL_CAR(mgr, cust, id) \
    manager_cancelCar(TM_ARG  mgr, cust, id)
#define MANAGER_CANCEL_ROOM(mgr, cust, id) \
    manager_cancelRoom(TM_ARG  mgr, cust, id)
#define MANAGER_CANCEL_FLIGHT(mgr, cust, id) \
    manager_cancelFlight(TM_ARG  mgr, cust, id)

}

#endif /* MANAGER_H */


/* =============================================================================
 *
 * End of manager.h
 *
 * =============================================================================
 */
