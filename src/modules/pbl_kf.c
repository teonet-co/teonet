/** 
 * File:   pbl_kf.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * PBL KeyFile module
 *
 * Created on August 20, 2015, 4:33 PM
 */

#include <stdlib.h>

#include "pbl_kf.h"

/**
 * Initialize PBL KeyFile module
 * 
 * @param ke
 * @return 
 */
ksnPblKfClass *ksnPblKfInit(void *ke) {
    
    ksnPblKfClass *kf = malloc(sizeof(ksnPblKfClass));
    kf->ke = ke;
    
    return kf;
}

/**
 * Destroy PBL KeyFile module
 * 
 * @param kf
 * @return 
 */
void ksnPblKfDestroy(ksnPblKfClass *kf) {
    
    if(kf != NULL) {
        free(kf);
    }
}
