/** 
 * File:   net_multi.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Module to manage multi teonet networks (host, client or proxy)
 * 
 * Created on July 23, 2015, 11:46 AM
 */

#include <stdlib.h>

#include "net_multi.h"

/**
 * Initialize ksnMultiClass object
 * 
 * @param km
 */
ksnMultiClass *ksnMultiInit(ksnMultiData *md) {
    
    ksnMultiClass *km = malloc(sizeof(ksnMultiClass));
    
    return km;
}

/**
 * Destroy ksnMultiClass object
 * 
 * @param km
 */
void ksnMultiDestroy(ksnMultiClass *km) {
    
    if(km != NULL) {
        free(km);
    }
}
