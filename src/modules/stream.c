/** 
 * File:   stream.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet stream module
 *
 * Created on October 3, 2015, 3:01 PM
 */

#include <stdlib.h>

#include "stream.h"

/**
 * Initialize Stream module
 * 
 * @param ke Pointer to 
 * @return 
 */
ksnStreamClass *ksnStreamInit(void *ke) {
    
    ksnStreamClass *ks = malloc(sizeof(ksnStreamClass));
    ks->ke = ke;
    
    return ks;
}

/**
 * Stream module destroy
 * 
 * @param ks Pointer to ksnStreamClass
 */
void ksnStreamDestroy(ksnStreamClass *ks) {
    
    if(ks != NULL) {
        free(ks);    
    }
}
