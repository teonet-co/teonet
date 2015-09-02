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
    
    int retval = -1;
    size_t data_len;
    
    ksnCQueData *cq = pblMapGet(kq->cque_map, &id, sizeof(id), &data_len);    
    if(cq != NULL) {
        
        // Execute queue callback
        cq->cb(id, 1, cq->data); // Type 1: successful callback
        
        // Remove record from queue
        if(pblMapRemove(kq->cque_map, &id, sizeof(id), &data_len) != (void*)-1) {
            
            retval = 0;
        }
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

    // Stop this timer watcher
    ev_timer_stop(EV_A_ w);

    // Execute queue callback
    cq->cb(cq->id, 0, cq->data); // Type 0: timeout callback
    
    // Remove record from Callback Queue
    size_t data_len;
    if(pblMapRemove(cq->kq->cque_map, &cq->id, sizeof(cq->id), 
            &data_len) != (void*)-1) {
            
        // Do something in success 
    }
    
    #undef cq
}

/**
 * Add callback to queue
 * 
 * @param kq Pointer to ksnCQueClass
 * @param cb Callback [function](@ref ksnCQueCallback)
 * @param timeout Callback timeout. If equal to 0 than timeout sets automatically
 * @param data The user data which should be send to the Callback function
 * 
 * @return Pointer to added ksnCQueData or NULL if error occurred
 */
ksnCQueData *ksnCQueAdd(ksnCQueClass *kq, ksnCQueCallback cb, double timeout, void *data) {
    
    ksnCQueData data_new, *cq = NULL; // Create CQue data buffer
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
        if(cq != NULL) {
            
            // Initialize, set user data and start the timer
            ev_timer_init(&cq->w, cq_timer_cb, timeout, 0.0);
            cq->w.data = cq; // Watcher data link to the ksnCQueData
            ev_timer_start(((ksnetEvMgrClass*)(kq->ke))->ev_loop, &cq->w);
        }
    }
    
    return cq;
}
