/* =============================================================================
 *
 * yada.c
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

#include "rapl.h"
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "element-htm.h"
#include "element-stm.h"
#include "region-stm.h"
#include "region-htm.h"
#include "list-stm.h"
#include "list-htm.h"
#include "mesh-stm.h"
#include "mesh-htm.h"
#include "heap-stm.h"
#include "heap-htm.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"

using namespace element_htm;
using namespace list_htm;
using namespace heap_htm;
using namespace mesh_htm;
using namespace region_htm;

#define PARAM_DEFAULT_INPUTPREFIX ("")
#define PARAM_DEFAULT_NUMTHREAD   (1L)
#define PARAM_DEFAULT_ANGLE       (20.0)
#define PARAM_DEFAULT_REPEATS (1)


char*    global_inputPrefix     = PARAM_DEFAULT_INPUTPREFIX;
long     global_numThread       = PARAM_DEFAULT_NUMTHREAD;
double   global_angleConstraint = PARAM_DEFAULT_ANGLE;
int      global_repeats = PARAM_DEFAULT_REPEATS;
mesh_t*  global_meshPtr;
heap_t*  global_workHeapPtr;
long     global_totalNumAdded = 0;
long     global_numProcess    = 0;


/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void
displayUsage (const char* appName)
{
    printf("Usage: %s [options]\n", appName);
    puts("\nOptions:                              (defaults)\n");
    printf("    a <FLT>   Min [a]ngle constraint  (%lf)\n", PARAM_DEFAULT_ANGLE);
    printf("    i <STR>   [i]nput name prefix     (%s)\n",  PARAM_DEFAULT_INPUTPREFIX);
    printf("    t <UINT>  Number of [t]hreads     (%li)\n", PARAM_DEFAULT_NUMTHREAD);
    exit(1);
}


/* =============================================================================
 * parseArgs
 * =============================================================================
 */
static void
parseArgs (long argc, char* const argv[])
{
    long i;
    long opt;

    opterr = 0;

    while ((opt = getopt(argc, argv, "a:i:t:r:")) != -1) {
        switch (opt) {
            case 'a':
                global_angleConstraint = atof(optarg);
                break;
            case 'i':
                global_inputPrefix = optarg;
                break;
            case 'r':
                global_repeats = atoi(optarg);
            case 't':
                global_numThread = atol(optarg);
                break;
            case '?':
            default:
                opterr++;
                break;
        }
    }

    for (i = optind; i < argc; i++) {
        fprintf(stderr, "Non-option argument: %s\n", argv[i]);
        opterr++;
    }

    if (opterr) {
        displayUsage(argv[0]);
    }
}


/* =============================================================================
 * initializeWork
 * =============================================================================
 */
static long
initializeWork (heap_t* workHeapPtr, mesh_t* meshPtr)
{
    random_t* randomPtr = random_alloc();
    random_seed(randomPtr, 0);
    mesh_htm::mesh_shuffleBad(meshPtr, randomPtr);
    random_free(randomPtr);

    long numBad = 0;

    while (1) {
        element_t* elementPtr = mesh_htm::mesh_getBad(meshPtr);
        if (!elementPtr) {
            break;
        }
        numBad++;
        bool_t status = heap_htm::heap_insert(workHeapPtr, (void*)elementPtr);
        assert(status);
        element_htm::element_setIsReferenced(elementPtr, TRUE);
    }

    return numBad;
}


/* =============================================================================
 * process
 * =============================================================================
 */
void
process ()
{
    TM_THREAD_ENTER();

    heap_t* workHeapPtr = global_workHeapPtr;
    mesh_t* meshPtr = global_meshPtr;
    region_t* regionPtr;
    long totalNumAdded = 0;
    long numProcess = 0;

    regionPtr = PREGION_ALLOC();
    assert(regionPtr);

    while (1) {

        element_t* elementPtr;

        int mode = 0;
        TM_BEGIN(0,mode);
        if (mode == 0) {
        	elementPtr = heap_htm::TMheap_remove(TM_ARG workHeapPtr);
        } else {
        	elementPtr = heap_stm::TMheap_remove(TM_ARG workHeapPtr);
        }
        TM_END();
        if (elementPtr == NULL) {
            break;
        }

        bool_t isGarbage;
        mode = 0;
        TM_BEGIN(1, mode);
        if (mode == 0) {
        	isGarbage = element_htm::TMelement_isGarbage(TM_ARG elementPtr);
        } else {
        	isGarbage = element_stm::TMelement_isGarbage(TM_ARG elementPtr);
        }
        TM_END();
        if (isGarbage) {
            /*
             * Handle delayed deallocation
             */
            PELEMENT_FREE(elementPtr);
            continue;
        }

        long numAdded;

        mode = 0;
        TM_BEGIN(2,mode);
        if (mode == 0) {
        	PREGION_CLEARBAD(regionPtr);
        	numAdded = region_htm::TMREGION_REFINE(regionPtr, elementPtr, meshPtr);
        } else {
        	PREGION_CLEARBAD(regionPtr);
        	numAdded = region_stm::TMREGION_REFINE(regionPtr, elementPtr, meshPtr);
        }
        TM_END();

        mode = 0;
        TM_BEGIN(3,mode);
        if (mode == 0) {
        	element_htm::TMELEMENT_SETISREFERENCED(elementPtr, FALSE);
        	isGarbage = element_htm::TMELEMENT_ISGARBAGE(elementPtr);
        } else {
        	element_stm::TMELEMENT_SETISREFERENCED(elementPtr, FALSE);
        	isGarbage = element_stm::TMELEMENT_ISGARBAGE(elementPtr);
        }
        TM_END();
        if (isGarbage) {
            /*
             * Handle delayed deallocation
             */
            PELEMENT_FREE(elementPtr);
        }

        totalNumAdded += numAdded;

        mode = 0;
        TM_BEGIN(4,mode);
        if (mode == 0) {
        	region_htm::TMREGION_TRANSFERBAD(regionPtr, workHeapPtr);
        } else {
        	region_stm::TMREGION_TRANSFERBAD(regionPtr, workHeapPtr);
        }
        TM_END();

        numProcess++;

    }

    int mode = 0;
    TM_BEGIN(5,mode);
    if (mode == 0) {
        FAST_PATH_SHARED_WRITE(global_totalNumAdded,
        		FAST_PATH_SHARED_READ(global_totalNumAdded) + totalNumAdded);
        FAST_PATH_SHARED_WRITE(global_numProcess,
        		FAST_PATH_SHARED_READ(global_numProcess) + numProcess);
    } else {
    	SLOW_PATH_SHARED_WRITE(global_totalNumAdded,
    			SLOW_PATH_SHARED_READ(global_totalNumAdded) + totalNumAdded);
    	SLOW_PATH_SHARED_WRITE(global_numProcess,
    			SLOW_PATH_SHARED_READ(global_numProcess) + numProcess);
    }
    TM_END();

    PREGION_FREE(regionPtr);

    TM_THREAD_EXIT();
}


/* =============================================================================
 * main
 * =============================================================================
 */
MAIN(argc, argv)
{
    GOTO_REAL();

    SETUP_NUMBER_TASKS(6);

    /*
     * Initialization
     */

    parseArgs(argc, (char** const)argv);
    SIM_GET_NUM_CPU(global_numThread);
    TM_STARTUP(global_numThread, YADA_ID);
    P_MEMORY_STARTUP(global_numThread);

    SETUP_NUMBER_THREADS(global_numThread);

    thread_startup(global_numThread);


int repeat = global_repeats;
double time_total = 0.0;
double energy_total = 0.0;
for (; repeat > 0; --repeat) {

    global_meshPtr = mesh_htm::mesh_alloc();
    assert(global_meshPtr);
    long initNumElement = mesh_htm::mesh_read(global_meshPtr, global_inputPrefix);
    global_workHeapPtr = heap_htm::heap_alloc(1, &element_htm::element_heapCompare);
    assert(global_workHeapPtr);
    long initNumBadElement = initializeWork(global_workHeapPtr, global_meshPtr);

    TIMER_T start;
startEnergyIntel();
    TIMER_READ(start);
    GOTO_SIM();
    thread_start(process, NULL);
    GOTO_REAL();
    TIMER_T stop;
    TIMER_READ(stop);
double time_tmp = TIMER_DIFF_SECONDS(start, stop);
double energy_tmp = endEnergyIntel();
printf("%lf\t%lf\n", time_tmp, energy_tmp);
PRINT_STATS();
time_total += time_tmp;
energy_total += energy_tmp;

}
    printf("Elapsed time                    = %0.3lf\n",
           time_total);
printf("Energy = %0.6lf\n", energy_total);
    fflush(stdout);

    TM_SHUTDOWN();
    P_MEMORY_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();

    MAIN_RETURN(0);
}


/* =============================================================================
 *
 * End of ruppert.c
 *
 * =============================================================================
 */
