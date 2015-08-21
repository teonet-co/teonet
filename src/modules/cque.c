/** 
 * File:   cque.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Module to manage Callback QUEUE: ksnCQue
 * 
 * Main module functions:
 * 
 *  * Initialize / Destroy module (ksnCQueClass)
 *  * Add callback to QUEUE, get peers TR-UDP (or ARP) TRIP_TIME and start timeout timer
 *  * Execute callback when callback event or timeout event occurred and Remove record from queue
 *
 * Created on August 21, 2015, 12:10 PM
 */

#include <stdlib.h>

#include "cque.h"

/**
 * Initialize ksnCQue module class
 * 
 * @param ke Pointer to ksnEvMgrClass
 * @return 
 */
ksnCQue *ksnCQueInit(void *ke) {
    
    ksnCQue *kq = malloc(sizeof(ksnCQue));
    kq->ke = ke;
    
    return kq;
}

/**
 * Destroy ksnCQue module class
 * 
 * @param kq
 */
void ksnCQueDestroy(ksnCQue *kq) {

    if(kq != NULL) {
        free(kq);
    }
}
