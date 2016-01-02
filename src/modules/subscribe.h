/** 
 * File:   subscribe.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on December 28, 2015, 1:31 PM
 */

#ifndef SUBSCRIBE_H
#define	SUBSCRIBE_H

#include <stdint.h>
#include <pbl.h>

/**
 * teoSScr Class structure definition
 */
typedef struct teoSScrClass {
    
    void *ke; ///< Pointer to ksnetEvMgrClass
//    PblList *list; ///< Pointer to the subscribers map
    PblMap *map; ///< Pointer to the subscribers map
    
} teoSScrClass;

#ifdef	__cplusplus
extern "C" {
#endif

teoSScrClass *teoSScrInit(void *ke);
void teoSScrDestroy(teoSScrClass *sscr);
void teoSScrSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev);
void teoSScrUnSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev);
void teoSScrSubscribe(teoSScrClass *sscr, char *peer_name, uint16_t ev);
void teoSScrUnSubscribe(teoSScrClass *sscr, char *peer_name, uint16_t ev);
void teoSScrSend(teoSScrClass *sscr, uint16_t ev, void *data, size_t data_length);

#ifdef	__cplusplus
}
#endif

#endif	/* SUBSCRIBE_H */
