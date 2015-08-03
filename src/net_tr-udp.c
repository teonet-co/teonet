/** 
 * File:   net_tr-udp.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet Real time communications over UDP protocol (TR-UDP)
 *
 * Created on August 4, 2015, 12:16 AM
 */

#include <stdlib.h>

#include "net_tr-udp.h"

/**
 * Initialize TR-UDP class
 * 
 * @param kc
 * @return 
 */
ksnTrUdpClass *ksnTrUdpInit(void *kc) {
    
    ksnTrUdpClass *tu = malloc(sizeof(ksnTrUdpClass));
            
    return tu;
}

/**
 * Destroy TR-UDP class
 * 
 * @param tu
 */
void ksnTrUdpDestroy(ksnTrUdpClass *tu) {
    
    if(tu != NULL) {
        free(tu);
    }
}
