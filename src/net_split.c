/** 
 * File:   net_split.c
 * Author: Kirill Scherba
 *
 * Created on July 17, 2015, 10:20 AM
 * 
 */

#include <stdlib.h>
#include <stdint.h>

#include "net_split.h" 

/**
 * Initialize split module
 * 
 * @param kc
 * @return 
 */
ksnSplitClass *ksnSplitInit(ksnCommandClass *kc) {
    
    ksnSplitClass *ks = malloc(sizeof(ksnSplitClass));
    ks->kc = kc;
    
    return ks;
}

/**
 * Destroy split module
 * @param ks
 */
void ksnSplitDestroy(ksnSplitClass *ks) {
    
    if(ks != NULL) {
        
        free(ks);
    }
}
