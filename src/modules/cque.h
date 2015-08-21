/** 
 * File:   cque.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on August 21, 2015, 12:10 PM
 */

#ifndef CQUE_H
#define	CQUE_H

#include <pbl.h>

/**
 * ksnCQue data structure
 */
typedef struct ksnCQueData {
    
    void *callback;
    char data[];
    
} ksnCQueData;

/**
 * ksnCQue Class structure definition
 */
typedef struct ksnCQueClass {
    
    void *ke; ///< Pointer to ksnEvMgrClass
    uint32_t id; ///< New callback queue ID
    PblMap *cque_map; ///< Pointer to the callback queue pblMap
    
} ksnCQueClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnCQueClass *ksnCQueInit(void *ke);
void ksnCQueDestroy(ksnCQueClass *kq);

int ksnCQueExec(ksnCQueClass *kq, uint32_t id);


#ifdef	__cplusplus
}
#endif

#endif	/* CQUE_H */
