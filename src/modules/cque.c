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
        
        // TODO: Execute callback cq->callback 
        
        // Remove record from queue
        if(pblMapRemove(kq->cque_map, &id, sizeof(id), &data_len) != (void*)-1) {
            
            retval = 0;
        }
    }
            
    return retval;
}

/**
 * Add callback to queue
 * 
 * @param kq
 * @param callback
 * 
 * @return Pointer to added ksnCQueData or NULL if error occurred
 */
ksnCQueData *ksnCQueAdd(ksnCQueClass *kq, void *callback) {
    
    ksnCQueData data, *cq = NULL; // Create CQue data buffer
    uint32_t id = kq->id++; // Get new ID
    size_t data_len; // Length of data
    
    data.callback = callback;
    
    if(pblMapAdd(kq->cque_map, &id, sizeof(id), &data, sizeof(data)) >= 0) {
        
        // Successfully added
        cq = pblMapGet(kq->cque_map, &id, sizeof(id), &data_len);
        if(cq != NULL) {
            
            // TODO: Create timeout watcher
        }
    }
    
    return cq;
}
