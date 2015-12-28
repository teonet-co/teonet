/** 
 * File:   subscribe.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on December 28, 2015, 1:31 PM
 */

#ifndef SUBSCRIBE_H
#define	SUBSCRIBE_H

#include "cque.h"

typedef struct teoSScrClass {
    
    void *ke; ///< Pointer to ksnetEvMgrClass
    PblMap *map; ///< Pointer to the subscribers map
    ksnCQueClass *cq; ///< Pointer to callback queue
    
} teoSScrClass;

#ifdef	__cplusplus
extern "C" {
#endif

teoSScrClass *teoSScrInit(void *ke);
void teoSScrDestroy(teoSScrClass *sscr);
void teoSScrSend(teoSScrClass *sscr, uint16_t ev, void *data, size_t data_length);

#ifdef	__cplusplus
}
#endif

#endif	/* SUBSCRIBE_H */

