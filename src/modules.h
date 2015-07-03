/** 
 * File:   modules.h
 * Author: Kirill Scherba
 *
 * Created on July 3, 2015, 12:25 PM
 */

#ifndef MODULES_H
#define	MODULES_H

#ifdef	__cplusplus
extern "C" {
#endif

    
/**
 * Modules class data
 */
typedef struct ksnModulesClass  {
    
    void *ke; ///< Pointer to Event manager class object
    
} ksnModulesClass;


ksnModulesClass *ksnModulesInit(void *ke);
void ksnModulesDestroy(ksnModulesClass **km);

#ifdef	__cplusplus
}
#endif

#endif	/* MODULES_H */
