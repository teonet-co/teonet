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
 * 
 * @return Pointer to created ksnCQueClass or NULL if error occurred
 */
ksnCQueClass *ksnCQueInit(void *ke) {
    
    ksnCQueClass *kq = malloc(sizeof(ksnCQueClass));
    if(kq != NULL) {
        kq->ke = ke; // Pointer event manager class
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
        if(kev->event_cb != NULL) 
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
    if(kev2->event_cb != NULL) 
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
        cq->timeout = timeout;
        if(cq != NULL && timeout > 0.0) {
            
            // Initialize, set user data and start the timer
            ev_timer_init(&cq->w, cq_timer_cb, timeout, 0.0);
            cq->w.data = cq; // Watcher data link to the ksnCQueData
            ev_timer_start(kev->ev_loop, &cq->w);
        }
    }
    
    return cq;
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
    }
    
    return rv;
}

#undef kev            
