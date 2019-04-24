/**
 * \file   cque.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * ### Manage Callback QUEUE class ksnCQueClass module
 *
 * Callback QUEUE used to organize async calls to any functions.
 *
 * #### Main module functions:
 *
 * * [Initialize](@ref ksnCQueInit) and [Destroy](@ref ksnCQueDestroy) Callback
 *   QUEUE module [class](@ref ksnCQueClass) (it done automatic when event
 *   manager initialized or destroyed)
 * * [Add callback](@ref ksnCQueAdd) to QUEUE, get peers TR-UDP (or ARP)
 *   TRIP_TIME and start [timeout timer](@ref cq_timer_cb)
 * * [Execute callback](@ref ksnCQueExec) when callback event or timeout event
 *   occurred and Remove record from queue
 *
 * See example: @ref teocque.c
 *
 * See test: test_cque.c
 *
 * Created on August 21, 2015, 12:10 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ev_mgr.h"
#include "cque.h"

#define kev ((ksnetEvMgrClass*)(kq->ke))

/**
 * Initialize ksnCQue module [class](@ref ksnCQueClass)
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param send_event Send cque event if true
 *
 * @return Pointer to created ksnCQueClass or NULL if error occurred
 */
ksnCQueClass *ksnCQueInit(void *ke, uint8_t send_event) {

    ksnCQueClass *kq = malloc(sizeof(ksnCQueClass));
    if(kq != NULL) {
        kq->ke = ke; // Pointer event manager class
        kq->event_f = send_event ? 1 : 0; // Send cque event if true
        kq->cque_map = pblMapNewHashMap(); // Create a new hash map
        kq->id = 0; // New callback queue id
    }

    return kq;
}

/**
 * Destroy ksnCQue module [class](@ref ksnCQueClass)
 *
 * @param kq Pointer to ksnCQueClass
 */
void ksnCQueDestroy(ksnCQueClass *kq) {

    if(kq != NULL) {

        pblMapFree(kq->cque_map);
        free(kq);
    }
}

/**
 * Execute callback queue record
 *
 * Get callback queue record and remove it from queue
 *
 * @param kq Pointer to ksnCQueClass
 * @param id Required ID
 *
 * @return return 0: if callback executed OK; !=0 some error occurred
 */
int ksnCQueExec(ksnCQueClass *kq, uint32_t id) {

    const int type = 1;
    int retval = -1;
    size_t data_len;

    ksnCQueData *cq = pblMapGet(kq->cque_map, &id, sizeof(id), &data_len);
    if(cq != NULL) {

        // Stop watcher
        if(cq->timeout > 0.0)
            ev_timer_stop(kev->ev_loop, &cq->w);

        // Execute queue callback
        if(cq->cb != NULL)
            cq->cb(id, type, cq->data); // Type 1: successful callback

        // Send teonet successful event in addition to callback
        if(kev->event_cb != NULL && cq->kq->event_f)
            kev->event_cb(kev, EV_K_CQUE_CALLBACK, cq, sizeof(ksnCQueData),
                    (void*)&type);

        // Remove record from queue
        if(pblMapRemoveFree(kq->cque_map, &id, sizeof(id), &data_len) != (void*)-1) {

            retval = 0;
        }
    }

    return retval;
}

/**
 * Remove callback from queue
 *
 * @param kq Pointer to ksnCQueClass
 * @param id Required ID
 *
 * @return return 0: if callback removed OK; !=0 some error occurred
 */
int ksnCQueRemove(ksnCQueClass *kq, uint32_t id) {
    int retval = -1;
    size_t data_len;

    ksnCQueData *cq = pblMapGet(kq->cque_map, &id, sizeof(id), &data_len);
    if(cq != NULL) {

        // Stop watcher
        if(cq->timeout > 0.0)
            ev_timer_stop(kev->ev_loop, &cq->w);

        // Remove record from queue
        if(pblMapRemoveFree(kq->cque_map, &id, sizeof(id), &data_len) != (void*)-1) {

            retval = 0;
        }
    }

    return retval;
}

/**
 * Set callback queue data, update data set in ksnCQueAdd
 *
 * @param kq Pointer to ksnCQueClass
 * @param id Existing callback queue ID
 * @param data Pointer to callback queue records data
 *
 * @return return 0: if callback executed OK; !=0 some error occurred
 */
int ksnCQueSetData(ksnCQueClass *kq, uint32_t id, void *data) {

    int retval = -1;
    size_t data_len;
    ksnCQueData *cq = pblMapGet(kq->cque_map, &id, sizeof(id), &data_len);
    if(cq != NULL) {

       cq->data = data;
        retval = 0;
    }

    return retval;
}

/**
 * Get callback queue data
 *
 * @param kq Pointer to ksnCQueClass
 * @param id Existing callback queue ID
 *
 * @return return Pointer to callback queue records data
 */
void * ksnCQueGetData(ksnCQueClass *kq, uint32_t id) {

    void * retval = NULL;
    size_t data_len;
    ksnCQueData *cq = pblMapGet(kq->cque_map, &id, sizeof(id), &data_len);
    if(cq != NULL) {

       retval = cq->data;
    }

    return retval;
}

/**
 * Callback Queue timeout timer callback
 *
 * @param w Event manager loop and watcher
 * @param revents Reserved (not used)
 */
void cq_timer_cb(EV_P_ ev_timer *w, int revents) {

    #define cq ((ksnCQueData*)(w->data))

    const int type = 0;

    // Stop this timer watcher
    ev_timer_stop(EV_A_ w);

    // Execute queue callback
    if(cq->cb != NULL)
        cq->cb(cq->id, type, cq->data); // Type 0: timeout callback

    // Send teonet timeout event in addition to callback
    #define kev2 ((ksnetEvMgrClass*)(cq->kq->ke))
    if(kev2->event_cb != NULL && cq->kq->event_f)
        kev2->event_cb(kev2, EV_K_CQUE_CALLBACK, cq, sizeof(ksnCQueData),
                (void*)&type);
    #undef kev2

    // Remove record from Callback Queue
    size_t data_len;
    if(pblMapRemoveFree(cq->kq->cque_map, &cq->id, sizeof(cq->id),
            &data_len) != (void*)-1) {

        // Do something in success
    }

    #undef cq
}

/**
 * Add callback to queue
 *
 * @param kq Pointer to ksnCQueClass
 * @param cb Callback [function](@ref ksnCQueCallback) or NULL. The teonet event
 *           EV_K_CQUE_CALLBACK should be send at the same time.
 * @param timeout Callback timeout. If equal to 0 than timeout sets automatically
 * @param data The user data which should be send to the Callback function
 *
 * @return Pointer to added ksnCQueData or NULL if error occurred
 */
ksnCQueData *ksnCQueAdd(ksnCQueClass *kq, ksnCQueCallback cb, double timeout,
        void *data) {

    ksnCQueData data_new, *cq = NULL; // Create CQue data buffer
    if(!kq->id) kq->id++; // Skip ID = 0
    uint32_t id = kq->id++; // Get new ID
    size_t data_len; // Length of data

    // Set Callback Queue data
    data_new.kq = kq; // ksnCQueClass
    data_new.id = id; // ID
    data_new.cb = cb; // Callback
    data_new.data = data; // User data

    // Add data to the Callback Queue
    if(pblMapAdd(kq->cque_map, &id, sizeof(id), &data_new, sizeof(data_new)) >= 0) {

        // If successfully added get real ksnCQueData pointer and start timeout
        // watcher
        cq = pblMapGet(kq->cque_map, &id, sizeof(id), &data_len);
        if(cq != NULL && timeout > 0.0) {
            // Initialize, set user data and start the timer
            cq->timeout = timeout;
            ev_timer_init(&cq->w, cq_timer_cb, timeout, 0.0);
            cq->w.data = cq; // Watcher data link to the ksnCQueData
            ev_timer_start(kev->ev_loop, &cq->w);
        }
    }

    return cq;
}

/**
 * Find data in CQue
 *
 * @param kq Pointer to ksnCQueClass
 * @param find Pointer to data to find
 * @param compare Callback compare function int compare(void* find, void* data)
 *                which return pointer to key if data find or NULL in other way
 * @param key_length [out] Pointer to key_length variable or NULL if not need it
 * @return
 */
void *ksnCQueFindData(ksnCQueClass *kq, void* find, ksnCQueCompare compare, size_t *key_length) {

    void *key = NULL;
    PblIterator *it = pblMapIteratorNew(kq->cque_map);
    if(it != NULL) {
        while(pblIteratorHasNext(it)) {
            void *entry = pblIteratorNext(it);
            ksnCQueData *data =  pblMapEntryValue(entry);
            if(compare(find, data->data)) {
                key = pblMapEntryKey(entry);
                if(key_length) *key_length = pblMapEntryKeyLength(entry);
                break;
            }
        }
        pblIteratorFree(it);
    }
    return key;
}

/**
 * Removes the mapping for this key from this map if it is present.
 *
 * @param map
 * @param key
 * @param keyLength
 * @param valueLengthPtr
 * @return
 * @retval != NULL && != (void*)-1: successfully removed
 * @retval == NULL: there was no mapping for key
 * @retval == (void*)-1: An error, see pbl_errno:
 *                       <BR>PBL_ERROR_OUT_OF_MEMORY - Out of memory.
 */
void *pblMapRemoveFree(PblMap * map, void * key, size_t keyLength,
        size_t * valueLengthPtr ) {

    void *rv = pblMapRemove(map, key, keyLength, valueLengthPtr);
    if(rv != NULL && rv != (void*)-1) {
        free(rv);
        // Maybe add this line: return (void *) 1;
    }

    return rv;
}

#undef kev
