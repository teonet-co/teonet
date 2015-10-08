/** 
 * File:   l0-server.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * L0 Server module
 *
 * Created on October 8, 2015, 1:38 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "l0-server.h"

/**
 * Initialize ksnL0s module [class](@ref ksnL0sClass)
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * 
 * @return Pointer to created ksnL0Class or NULL if error occurred
 */
ksnL0sClass *ksnL0sInit(void *ke) {
    
    ksnL0sClass *kl = malloc(sizeof(ksnL0sClass));
    if(kl != NULL)  {
        kl->ke = ke; // Pointer event manager class
        kl->arp = pblMapNewHashMap(); // Create a new hash map
    }
    
    return kl;
}

/**
 * Destroy ksnL0s module [class](@ref ksnL0sClass)
 * 
 * @param kl Pointer to ksnL0sClass
 */
void ksnL0sDestroy(ksnL0sClass *kl) {
    
    if(kl != NULL) {
        free(kl->arp);
        free(kl);
    }
}
