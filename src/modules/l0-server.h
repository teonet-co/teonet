/** 
 * File:   l0-server.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on October 8, 2015, 1:39 PM
 */

#ifndef L0_SERVER_H
#define	L0_SERVER_H

#include <pbl.h>

/**
 * ksnCksnL0 Class structure definition
 */
typedef struct  ksnL0sClass {
    
    void *ke;       ///< Pointer to ksnEvMgrClass
    PblMap *arp;    ///< Pointer to the L0 clients map
    int fd;         ///< L0 TCP Server FD
    
} ksnL0sClass;

#ifdef	__cplusplus
extern "C" {
#endif

ksnL0sClass *ksnL0sInit(void *ke);
void ksnL0sDestroy(ksnL0sClass *kl);

#ifdef	__cplusplus
}
#endif

#endif	/* L0_SERVER_H */
