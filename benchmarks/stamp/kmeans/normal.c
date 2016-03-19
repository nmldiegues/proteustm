/* =============================================================================
 *
 * normal.c
 * -- Implementation of normal k-means clustering algorithm
 *
 * =============================================================================
 *
 * Author:
 *
 * Wei-keng Liao
 * ECE Department, Northwestern University
 * email: wkliao@ece.northwestern.edu
 *
 *
 * Edited by:
 *
 * Jay Pisharath
 * Northwestern University.
 *
 * Chi Cao Minh
 * Stanford University
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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "normal.h"
#include "random.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"
#include "util.h"
#include <cstring>
#include "rapl.h"

double global_time = 0.0;
double global_energy = 0.0;

typedef struct args {
    double** feature;
    int     nfeatures;
    int     npoints;
    int     nclusters;
    int*    membership;
    double** clusters;
    int**   new_centers_len;
    double** new_centers;
} args_t;

double global_delta;
long global_i; /* index into task queue */

#define CHUNK 3


/* =============================================================================
 * work
 * =============================================================================
 */
static void
work (void* argPtr)
{
    TM_THREAD_ENTER();

    args_t* args = (args_t*)argPtr;
    double** feature         = args->feature;
    int     nfeatures       = args->nfeatures;
    int     npoints         = args->npoints;
    int     nclusters       = args->nclusters;
    int*    membership      = args->membership;
    double** clusters        = args->clusters;
    int**   new_centers_len = args->new_centers_len;
    double** new_centers     = args->new_centers;
    double delta = 0.0;
    int index;
    int i;
    int j;
    int start;
    int stop;
    int myId;

    myId = thread_getId();

    start = myId * CHUNK;

    while (start < npoints) {
        stop = (((start + CHUNK) < npoints) ? (start + CHUNK) : npoints);
        for (i = start; i < stop; i++) {

            index = common_findNearestPoint(feature[i],
                                            nfeatures,
                                            clusters,
                                            nclusters);
            /*
             * If membership changes, increase delta by 1.
             * membership[i] cannot be changed by other threads
             */
            if (membership[i] != index) {
                delta += 1.0;
            }

            /* Assign the membership to object i */
            /* membership[i] can't be changed by other thread */
            membership[i] = index;

            /* Update new cluster centers : sum of objects located within */
            int mode = 0;
            TM_BEGIN(0,mode);
            if (mode == 0) {
                FAST_PATH_SHARED_WRITE(*new_centers_len[index],
                		FAST_PATH_SHARED_READ(*new_centers_len[index]) + 1);
                for (j = 0; j < nfeatures; j++) {
                	FAST_PATH_SHARED_WRITE_D(
                        new_centers[index][j],
                        (FAST_PATH_SHARED_READ_D(new_centers[index][j]) + feature[i][j])
                    );
                }
            } else {
            	SLOW_PATH_SHARED_WRITE(*new_centers_len[index],
            			SLOW_PATH_SHARED_READ(*new_centers_len[index]) + 1);
                for (j = 0; j < nfeatures; j++) {
                	SLOW_PATH_SHARED_WRITE_D(
                        new_centers[index][j],
                        (SLOW_PATH_SHARED_READ_D(new_centers[index][j]) + feature[i][j])
                    );
                }
            }
            TM_END();
        }

        /* Update task queue */
        if (start + CHUNK < npoints) {
        	int mode = 0;
            TM_BEGIN(1, mode);
            if (mode == 0) {
                start = (int)FAST_PATH_SHARED_READ(global_i);
                FAST_PATH_SHARED_WRITE(global_i, (start + CHUNK));
            } else {
                start = (int)SLOW_PATH_SHARED_READ(global_i);
                SLOW_PATH_SHARED_WRITE(global_i, (start + CHUNK));
            }
            TM_END();
        } else {
            break;
        }
    }

    int mode = 0;
    TM_BEGIN(2,mode);
    if (mode == 0) {
    	FAST_PATH_SHARED_WRITE_D(global_delta, FAST_PATH_SHARED_READ_D(global_delta) + delta);
    } else {
    	SLOW_PATH_SHARED_WRITE_D(global_delta, SLOW_PATH_SHARED_READ_D(global_delta) + delta);
    }
    TM_END();

    TM_THREAD_EXIT();
}


/* =============================================================================
 * normal_exec
 * =============================================================================
 */
double**
normal_exec (int       nthreads,
             double**   feature,    /* in: [npoints][nfeatures] */
             int       nfeatures,
             int       npoints,
             int       nclusters,
             double     threshold,
             int*      membership,
             random_t* randomPtr) /* out: [npoints] */
{
    int i;
    int j;
    int loop = 0;
    int** new_centers_len; /* [nclusters]: no. of points in each cluster */
    double delta;
    double** clusters;      /* out: [nclusters][nfeatures] */
    double** new_centers;   /* [nclusters][nfeatures] */
    void* alloc_memory = NULL;
    args_t args;
    TIMER_T start;
    TIMER_T stop;

double time_tmp;
double energy_tmp;

    /* Allocate space for returning variable clusters[] */
    clusters = (double**)malloc(nclusters * sizeof(double*));
    assert(clusters);
    clusters[0] = (double*)malloc(nclusters * nfeatures * sizeof(double));
    assert(clusters[0]);
    for (i = 1; i < nclusters; i++) {
        clusters[i] = clusters[i-1] + nfeatures;
    }

    /* Randomly pick cluster centers */
    for (i = 0; i < nclusters; i++) {
        int n = (int)(random_generate(randomPtr) % npoints);
        for (j = 0; j < nfeatures; j++) {
            clusters[i][j] = feature[n][j];
        }
    }

    for (i = 0; i < npoints; i++) {
        membership[i] = -1;
    }

    /*
     * Need to initialize new_centers_len and new_centers[0] to all 0.
     * Allocate clusters on different cache lines to reduce false sharing.
     */
    {
        int cluster_size = sizeof(int) + sizeof(double) * nfeatures;
        const int cacheLineSize = 32;
        cluster_size += (cacheLineSize-1) - ((cluster_size-1) % cacheLineSize);
        alloc_memory = malloc(nclusters * cluster_size);
        memset(alloc_memory, 0, nclusters * cluster_size);
        new_centers_len = (int**) malloc(nclusters * sizeof(int*));
        new_centers = (double**) malloc(nclusters * sizeof(double*));
        assert(alloc_memory && new_centers && new_centers_len);
        for (i = 0; i < nclusters; i++) {
            new_centers_len[i] = (int*)((char*)alloc_memory + cluster_size * i);
            new_centers[i] = (double*)((char*)alloc_memory + cluster_size * i + sizeof(int));
        }
    }

    TIMER_READ(start);
startEnergyIntel();

    GOTO_SIM();

    do {
        delta = 0.0;

        args.feature         = feature;
        args.nfeatures       = nfeatures;
        args.npoints         = npoints;
        args.nclusters       = nclusters;
        args.membership      = membership;
        args.clusters        = clusters;
        args.new_centers_len = new_centers_len;
        args.new_centers     = new_centers;

        global_i = nthreads * CHUNK;
        global_delta = delta;

#ifdef OTM
#pragma omp parallel
        {
            work(&args);
        }
#else
        thread_start(work, &args);
#endif

        delta = global_delta;

        /* Replace old cluster centers with new_centers */
        for (i = 0; i < nclusters; i++) {
            for (j = 0; j < nfeatures; j++) {
                if (new_centers_len[i] > 0) {
                    clusters[i][j] = new_centers[i][j] / *new_centers_len[i];
                }
                new_centers[i][j] = 0.0;   /* set back to 0 */
            }
            *new_centers_len[i] = 0;   /* set back to 0 */
        }

        delta /= npoints;

    } while ((delta > threshold) && (loop++ < 500));

    GOTO_REAL();

    TIMER_READ(stop);
time_tmp = TIMER_DIFF_SECONDS(start, stop);
    global_time += time_tmp;

 endEnergyIntelRet(&energy_tmp);
    global_energy += energy_tmp;


printf("%lf\t%lf\n", time_tmp, energy_tmp);
PRINT_STATS();

    free(alloc_memory);
    free(new_centers);
    free(new_centers_len);

    return clusters;
}


/* =============================================================================
 *
 * End of normal.c
 *
 * =============================================================================
 */
