/** 
 * File:   cque.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Module to manage Callback QUEUE: ksnCQue
 * 
 * Main module functions:
 * 
 *      * Initialize / Destroy module (ksnCQueClass)
 *      * Add callback to QUEUE, get peers TR-UDP (or ARP) TRIP_TIME and start 
 *        timeout timer
 *      * Execute callback when callback event or timeout event occurred and 
 *        Remove record from queue
 *
 * Created on August 21, 2015, 12:10 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ev_mgr.h"
#include "cque.h"

/**
 * Initialize ksnCQue module class
 * 
 * @param ke Pointer to ksnEvMgrClass
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
 * Destroy ksnCQue module class
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
 * @param kq Pointer to ksnCQue Class
 * @param id Required ID
 * @param data_length [out] Returned data length
 * 
 * @return return 0: if callback executed OK; !=0 some error occurred
 */
int ksnCQueExec(ksnCQueClass *kq, uint32_t id) {
    
    int retval = 0;
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
 * @param loop
 * @param w
 * @param revents
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
 * @param kq Pointer to ksnCQue Class
 * @param callback Callback function
 * @param timeout Callback timeout. If equal to 0 than timeout sets automatically
 * @param data  The user data which should be send to the callback function
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
