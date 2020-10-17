/**
 * File:   modules.c
 * Author: Kirill Scherba
 *
 * Created on July 3, 2015, 12:25 PM
 */

#include <stdlib.h>

#include "ev_mgr.h"
#include "modules.h"

// Local (private) functions
void ksnModuleLInit(ksnModulesClass *km);
void ksnModuleLDestroy(ksnModulesClass *km);
void ksnModulesLAdd(ksnModulesClass *km, ksnModuleElement *elements, int numer_of_elements);

/**
 * Initialize module
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param modules
 * @param numer_of_modules
 * 
 * @return Pointer to ksnModulesClass
 */
ksnModulesClass *ksnModulesInit(void *ke, ksnModuleElement *modules, int numer_of_modules) {

    // Allocate module and set defaults
    ksnModulesClass *km = malloc(sizeof(ksnModulesClass));
    km->modules = modules;
    ksnModuleLInit(km);
    km->ke = ke;
    
//    printf("ksnModulesInit\n");
//    
    // Add and initialize modules
    ksnModulesLAdd(km, modules, numer_of_modules);
           
    return km;
}

/**
 * Destroy module
 *
 * @param km
 */
void ksnModulesDestroy(ksnModulesClass *km) {

//    printf("ksnModulesDestroy\n");
//    
    if(km != NULL) {
        
        ksnetEvMgrClass *ke = km->ke;
        ksnModuleLDestroy(km);
        free(km);
        ke->km = NULL;
    }
}

/**
 * Initialize modules list
 */
void ksnModuleLInit(ksnModulesClass *km) {
    
    km->list = pblListNewArrayList();
}

/**
 * Destroy modules list
 */
void ksnModuleLDestroy(ksnModulesClass *km) {
    
    // Loop list and destroy all loaded modules
    PblIterator *it =  pblListIterator(km->list);
    if(it != NULL) {       
        while(pblIteratorHasNext(it)) {        
            
//            printf("ksnModuleLDestroy before destroy\n");
            ksnModuleElement *module = pblIteratorNext(it);
            module->destroy(module->mc);           
        }
        pblIteratorFree(it);
    }
    
    // Free modules array
    free(km->modules);
    
    // Free list
    pblListFree(km->list);
}

/**
 * Appends the specified element to the end of this list
 */
void ksnModulesLAdd(ksnModulesClass *km, ksnModuleElement *modules, int numer_of_elements) {
     
    int i;
    
    for(i = 0; i < numer_of_elements; i++) {
//        printf("ksnModulesLAdd before initialize - 0\n");
        pblListAdd(km->list, (void*) &modules[i]);
//        printf("ksnModulesLAdd before initialize - 1\n");
        modules[i].mc = modules[i].init(km->ke);
    }
}
