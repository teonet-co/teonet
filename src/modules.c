/** 
 * File:   modules.c
 * Author: Kirill Scherba
 *
 * Created on July 3, 2015, 12:25 PM
 */

#include <stdlib.h>

#include "ev_mgr.h"
#include "modules.h"

/**
 * Initialize module
 * 
 * @param ke
 * @return 
 */
ksnModulesClass *ksnModulesInit(void *ke) {
    
    ksnModulesClass *km = malloc(sizeof(ksnModulesClass));
    
    return km;
}

/**
 * Destroy module
 * 
 * @param km
 */
void ksnModulesDestroy(ksnModulesClass **km) {
    
    free(*km);
    km = NULL;
}

//void ksnModulesAdd();