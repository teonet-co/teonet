/**
 * File:   modules.h
 * Author: Kirill Scherba
 *
 * Created on July 3, 2015, 12:25 PM
 */

#ifndef MODULES_H
#define	MODULES_H

#include "config/conf.h"


#ifdef	__cplusplus
extern "C" {
#endif


/**
 * Module Element structure
 */
typedef struct ksnModuleElement {
    
    char name[KSN_BUFFER_SM_SIZE];
    void* (*init)(void *ke);
    void (*destroy)(void *mc);
    void *mc; ///< Module class
    
} ksnModuleElement;

/**
 * Modules class data
 */
typedef struct ksnModulesClass  {

    void *ke; ///< Pointer to Event manager class object
    PblList *list; ///< Modules list
    ksnModuleElement *modules; ///< Pointer to Modules array

} ksnModulesClass;

ksnModulesClass *ksnModulesInit(void *ke, ksnModuleElement* modules, int numer_of_modules);
void ksnModulesDestroy(ksnModulesClass *km);

#ifdef	__cplusplus
}
#endif

#endif	/* MODULES_H */
