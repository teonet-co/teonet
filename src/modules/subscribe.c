/** 
 * File:   subscribe.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on December 28, 2015, 1:31 PM
 */

/**
 * Subscribe module
 * 
 * Subscribe any events
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "subscribe.h"

/**
 * teoSScrClass map data
 */
typedef struct teoSScrMapData {
    
    ksnCQueData *cq;    ///< Pointer to CQUEUE Data
    
} teoSScrMapData;

///**
// * teoSScrClass map key
// */
//typedef struct teoSScrMapKey {
//    
//    uint16_t ev;    ///< Event
//    char name[];    ///< Peer name array
//    
//} teoSScrMapKey;


/**
 * Initialize teoSScr module
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @return Pointer to created teoSScrClass or NULL if error occurred
 */
teoSScrClass *teoSScrInit(void *ke) {
    
    teoSScrClass *sscr = malloc(sizeof(teoSScrClass));
    
    if(sscr != NULL) {
        sscr->map = pblMapNewHashMap(); // Create a new hash map
        sscr->cq = ksnCQueInit(ke); // Create CQueue Class
    }
    
    return sscr;
}

/**
 * Send data to subscribers
 * 
 * @param sscr
 * @param ev
 * @param data
 * @param data_length
 */
void teoSScrSend(teoSScrClass *sscr, uint16_t ev, void *data, size_t data_length) {

//    size_t sscr_key_len = sizeof(teoSScrMapKey) + strlen(peer_name) + 1;
//    teoSScrMapKey *sscr_key = malloc(sscr_key_len);
//    teoSScrMapData sscr_data;
    
    
}

/**
 * Subscribe to event
 * 
 * @param sscr
 * @param peer_name
 * @param ev
 * @param cb
 */
void teoSScrSubscribe(teoSScrClass *sscr, char *peer_name, uint16_t ev, 
        ksnCQueCallback cb) {
    
//    size_t sscr_key_len = sizeof(teoSScrMapKey) + strlen(peer_name) + 1;
//    teoSScrMapKey *sscr_key = malloc(sscr_key_len);
    teoSScrMapData sscr_data;
    
    sscr_data.cq = ksnCQueAdd(sscr->cq, cb, 0, NULL);
    pblMapAdd(sscr->map, &ev, sizeof(ev), &sscr_data, sizeof(sscr_data));
    
//    free(sscr_key);
}

/**
 * Destroy teoSScr module
 * 
 * @param sscr Pointer to ksnCQueClass
 */
void teoSScrDestroy(teoSScrClass *sscr) {

    if(sscr != NULL) {
        
        pblMapFree(sscr->map);
        ksnCQueDestroy(sscr->cq);
        free(sscr);
    }
}
