/* =============================================================================
 *
 * decoder.c
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
#include <string.h>
#include "decoder-stm-v2.h"
#include "list-stm-v2.h"
#include "rbtree-stm-v2.h"
#include "queue-stm-v2.h"
#include "error.h"
#include "packet.h"
#include "tm.h"
#include "types.h"

using namespace list_stm_v2;
using namespace rbtree_stm_v2;
using namespace queue_stm_v2;


/* =============================================================================
 * decoder_alloc
 * =============================================================================
 */
decoder_t*
decoder_stm_v2::decoder_alloc ()
{
    decoder_t* decoderPtr;

    decoderPtr = (decoder_t*)malloc(sizeof(decoder_t));
    if (decoderPtr) {
        decoderPtr->fragmentedMapPtr = MAP_ALLOC(NULL, NULL);
        assert(decoderPtr->fragmentedMapPtr);
        decoderPtr->decodedQueuePtr = queue_alloc(1024);
        assert(decoderPtr->decodedQueuePtr);
    }

    return decoderPtr;
}


/* =============================================================================
 * decoder_free
 * =============================================================================
 */
void
decoder_stm_v2::decoder_free (decoder_t* decoderPtr)
{
    queue_free(decoderPtr->decodedQueuePtr);
    MAP_FREE(decoderPtr->fragmentedMapPtr);
    free(decoderPtr);
}


/* =============================================================================
 * decoder_process
 * =============================================================================
 */
error_t
decoder_stm_v2::decoder_process (decoder_t* decoderPtr, char* bytes, long numByte)
{
    bool_t status;

    /*
     * Basic error checking
     */

    if (numByte < PACKET_HEADER_LENGTH) {
        return ERROR_SHORT;
    }

    packet_t* packetPtr = (packet_t*)bytes;
    long flowId      = packetPtr->flowId;
    long fragmentId  = packetPtr->fragmentId;
    long numFragment = packetPtr->numFragment;
    long length      = packetPtr->length;

    if (flowId < 0) {
        return ERROR_FLOWID;
    }

    if ((fragmentId < 0) || (fragmentId >= numFragment)) {
        return ERROR_FRAGMENTID;
    }

    if (length < 0) {
        return ERROR_LENGTH;
    }

#if 0
    /*
     * With the above checks, this one is redundant
     */
    if (numFragment < 1) {
        return ERROR_NUMFRAGMENT;
    }
#endif

    /*
     * Add to fragmented map for reassembling
     */

    if (numFragment > 1) {

        MAP_T* fragmentedMapPtr = decoderPtr->fragmentedMapPtr;
        list_t* fragmentListPtr =
            (list_t*)MAP_FIND(fragmentedMapPtr, (void*)flowId);

        if (fragmentListPtr == NULL) {

            fragmentListPtr = list_alloc(&packet_compareFragmentId);
            assert(fragmentListPtr);
            status = list_insert(fragmentListPtr, (void*)packetPtr);
            assert(status);
            status = MAP_INSERT(fragmentedMapPtr,
                                (void*)flowId,
                                (void*)fragmentListPtr);
            assert(status);

        } else {

            list_iter_t it;
            list_iter_reset(&it, fragmentListPtr);
            assert(list_iter_hasNext(&it, fragmentListPtr));
            packet_t* firstFragmentPtr =
                (packet_t*)list_iter_next(&it, fragmentListPtr);
            long expectedNumFragment = firstFragmentPtr->numFragment;

            if (numFragment != expectedNumFragment) {
                status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);
                return ERROR_NUMFRAGMENT;
            }

            status = list_insert(fragmentListPtr, (void*)packetPtr);
            assert(status);

            /*
             * If we have all the fragments we can reassemble them
             */

            if (list_getSize(fragmentListPtr) == numFragment) {

                long numByte = 0;
                long i = 0;
                list_iter_reset(&it, fragmentListPtr);
                while (list_iter_hasNext(&it, fragmentListPtr)) {
                    packet_t* fragmentPtr =
                        (packet_t*)list_iter_next(&it, fragmentListPtr);
                    assert(fragmentPtr->flowId == flowId);
                    if (fragmentPtr->fragmentId != i) {
                        status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                        assert(status);
                        return ERROR_INCOMPLETE; /* should be sequential */
                    }
                    numByte += fragmentPtr->length;
                    i++;
                }

                char* data = (char*)malloc(numByte + 1);
                assert(data);
                data[numByte] = '\0';
                char* dst = data;
                list_iter_reset(&it, fragmentListPtr);
                while (list_iter_hasNext(&it, fragmentListPtr)) {
                    packet_t* fragmentPtr =
                        (packet_t*)list_iter_next(&it, fragmentListPtr);
                    memcpy(dst, fragmentPtr->data, fragmentPtr->length);
                    dst += fragmentPtr->length;
                }
                assert(dst == data + numByte);

                decoded_t* decodedPtr = (decoded_t*)malloc(sizeof(decoded_t));
                assert(decodedPtr);
                decodedPtr->flowId = flowId;
                decodedPtr->data = data;

                queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
                status = queue_push(decodedQueuePtr, (void*)decodedPtr);
                assert(status);

                list_free(fragmentListPtr);
                status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);
            }

        }

    } else {

        /*
         * This is the only fragment, so it is ready
         */

        if (fragmentId != 0) {
            return ERROR_FRAGMENTID;
        }

        char* data = (char*)malloc(length + 1);
        assert(data);
        data[length] = '\0';
        memcpy(data, packetPtr->data, length);

        decoded_t* decodedPtr = (decoded_t*)malloc(sizeof(decoded_t));
        assert(decodedPtr);
        decodedPtr->flowId = flowId;
        decodedPtr->data = data;

        queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
        status = queue_push(decodedQueuePtr, (void*)decodedPtr);
        assert(status);

    }

    return ERROR_NONE;
}


/* =============================================================================
 * TMdecoder_process
 * =============================================================================
 */
error_t
decoder_stm_v2::TMdecoder_process (TM_ARGDECL  decoder_t* decoderPtr, char* bytes, long numByte)
{
    bool_t status;

    /*
     * Basic error checking
     */

    if (numByte < PACKET_HEADER_LENGTH) {
        return ERROR_SHORT;
    }

    packet_t* packetPtr = (packet_t*)bytes;
    long flowId      = packetPtr->flowId;
    long fragmentId  = packetPtr->fragmentId;
    long numFragment = packetPtr->numFragment;
    long length      = packetPtr->length;

    if (flowId < 0) {
        return ERROR_FLOWID;
    }

    if ((fragmentId < 0) || (fragmentId >= numFragment)) {
        return ERROR_FRAGMENTID;
    }

    if (length < 0) {
        return ERROR_LENGTH;
    }

#if 0
    /*
     * With the above checks, this one is redundant
     */
    if (numFragment < 1) {
        return ERROR_NUMFRAGMENT;
    }
#endif

    /*
     * Add to fragmented map for reassembling
     */

    if (numFragment > 1) {

        MAP_T* fragmentedMapPtr = decoderPtr->fragmentedMapPtr;
        list_t* fragmentListPtr =
            (list_t*)TMMAP_FIND(fragmentedMapPtr, (void*)flowId);

        if (fragmentListPtr == NULL) {

            fragmentListPtr = TMLIST_ALLOC(&packet_compareFragmentId);
            assert(fragmentListPtr);
            status = TMLIST_INSERT(fragmentListPtr, (void*)packetPtr);
            assert(status);
            status = TMMAP_INSERT(fragmentedMapPtr,
                                  (void*)flowId,
                                  (void*)fragmentListPtr);
            assert(status);

        } else {

            list_iter_t it;
            TMLIST_ITER_RESET(&it, fragmentListPtr);
            assert(TMLIST_ITER_HASNEXT(&it, fragmentListPtr));
            packet_t* firstFragmentPtr =
                (packet_t*)TMLIST_ITER_NEXT(&it, fragmentListPtr);
            long expectedNumFragment = firstFragmentPtr->numFragment;

            if (numFragment != expectedNumFragment) {
                status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);
                return ERROR_NUMFRAGMENT;
            }

            status = TMLIST_INSERT(fragmentListPtr, (void*)packetPtr);
            assert(status);

            /*
             * If we have all the fragments we can reassemble them
             */

            if (TMLIST_GETSIZE(fragmentListPtr) == numFragment) {

                long numByte = 0;
                long i = 0;
                TMLIST_ITER_RESET(&it, fragmentListPtr);
                while (TMLIST_ITER_HASNEXT(&it, fragmentListPtr)) {
                    packet_t* fragmentPtr =
                        (packet_t*)TMLIST_ITER_NEXT(&it, fragmentListPtr);
                    assert(fragmentPtr->flowId == flowId);
                    if (fragmentPtr->fragmentId != i) {
                        status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                        assert(status);
                        return ERROR_INCOMPLETE; /* should be sequential */
                    }
                    numByte += fragmentPtr->length;
                    i++;
                }

                char* data = (char*)TM_MALLOC(numByte + 1);
                assert(data);
                data[numByte] = '\0';
                char* dst = data;
                TMLIST_ITER_RESET(&it, fragmentListPtr);
                while (TMLIST_ITER_HASNEXT(&it, fragmentListPtr)) {
                    packet_t* fragmentPtr =
                        (packet_t*)TMLIST_ITER_NEXT(&it, fragmentListPtr);
                    memcpy(dst, fragmentPtr->data, fragmentPtr->length);
                    dst += fragmentPtr->length;
                }
                assert(dst == data + numByte);

                decoded_t* decodedPtr = (decoded_t*)TM_MALLOC(sizeof(decoded_t));
                assert(decodedPtr);
                decodedPtr->flowId = flowId;
                decodedPtr->data = data;

                queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
                status = TMQUEUE_PUSH(decodedQueuePtr, (void*)decodedPtr);
                assert(status);

                TMLIST_FREE(fragmentListPtr);
                status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);
            }

        }

    } else {

        /*
         * This is the only fragment, so it is ready
         */

        if (fragmentId != 0) {
            return ERROR_FRAGMENTID;
        }

        char* data = (char*)TM_MALLOC(length + 1);
        assert(data);
        data[length] = '\0';
        memcpy(data, packetPtr->data, length);

        decoded_t* decodedPtr = (decoded_t*)TM_MALLOC(sizeof(decoded_t));
        assert(decodedPtr);
        decodedPtr->flowId = flowId;
        decodedPtr->data = data;

        queue_t* decodedQueuePtr = decoderPtr->decodedQueuePtr;
        status = TMQUEUE_PUSH(decodedQueuePtr, (void*)decodedPtr);
        assert(status);

    }

    return ERROR_NONE;
}


/* =============================================================================
 * decoder_getComplete
 * -- If none, returns NULL
 * =============================================================================
 */
char*
decoder_stm_v2::decoder_getComplete (decoder_t* decoderPtr, long* decodedFlowIdPtr)
{
    char* data;
    decoded_t* decodedPtr = queue_pop(decoderPtr->decodedQueuePtr);

    if (decodedPtr) {
        *decodedFlowIdPtr = decodedPtr->flowId;
        data = decodedPtr->data;
        free(decodedPtr);
    } else {
        *decodedFlowIdPtr = -1;
        data = NULL;
    }

    return data;
}


/* =============================================================================
 * TMdecoder_getComplete
 * -- If none, returns NULL
 * =============================================================================
 */
char*
decoder_stm_v2::TMdecoder_getComplete (TM_ARGDECL  decoder_t* decoderPtr, long* decodedFlowIdPtr)
{
    char* data;
    decoded_t* decodedPtr = TMQUEUE_POP(decoderPtr->decodedQueuePtr);

    if (decodedPtr) {
        *decodedFlowIdPtr = decodedPtr->flowId;
        data = decodedPtr->data;
        SLOW_PATH_FREE(decodedPtr);
    } else {
        *decodedFlowIdPtr = -1;
        data = NULL;
    }

    return data;
}
