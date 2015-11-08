/** 
 * File:   teo_ws.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet websocket L0 connector module
 *
 * Created on November 8, 2015, 3:58 PM
 */

#include <stdlib.h>
#include "teo_ws.h"

// Local functions
void teoWSDestroy(teoWSClass *kws);
int teoWSevHandler(teoWSClass *kws, int ev, void *nc, void *data, size_t data_length);
int teoWSprocessMsg(teoWSClass *kws, void *nc, void *data, size_t data_length);

/**
 * Pointer to mg_connection structure
 */
#define nc ((struct mg_connection *)nc_p)

/**
 * Initialize teonet websocket module
 * 
 * @param kh Pointer to ksnHTTPClass
 * 
 * @return 
 */
teoWSClass* teoWSInit(ksnHTTPClass *kh) {
    
    teoWSClass *this = malloc(sizeof(teoWSClass));
    this->kh = kh;
    
    this->destroy = teoWSDestroy;
    this->handler = teoWSevHandler;
    this->processMsg = teoWSprocessMsg;
    
    return this;
}

/**
 * Destroy teonet HTTP module]
 * @param kws Pointer to teoWSClass
 */
void teoWSDestroy(teoWSClass *kws) {
    
    if(kws != NULL)
        free(kws);
}

/**
 * Teonet L0 websocket event handler
 * 
 * @param kws Pointer to teoWSClass
 * @param ev Event
 * @param nc_p Pointer to mg_connection structure
 * @param data Websocket data
 * @param data_length Websocket data length
 * 
 * @return True if event processed
 */
int teoWSevHandler(teoWSClass *kws, int ev, void *nc_p, void *data, 
        size_t data_length) {
    
    int processed = 0;
    
    // Check websocket events
    switch(ev) {
        
        // Websocket message.
        case MG_EV_WEBSOCKET_FRAME: {
            
            kws->processMsg(kws, nc_p, data, data_length);
                    
        } break;
        
        default:
            break;
    }
    
    return processed;
}

/**
 * Process websocket message
 * 
 * @param kws Pointer to teoWSClass
 * @param nc_p Pointer to mg_connection structure
 * @param data Websocket data
 * @param data_length Websocket data length
 * 
 * @return  True if message processed
 */
int teoWSprocessMsg(teoWSClass *kws, void *nc_p, void *data, 
        size_t data_length) {
    
    int processed = 0;
    
    // \todo
    
    // Check for L0 websocket Login command
    
    // Check for L0 websocket Peer command
    
    // Check for L0 websocket ECHO command
    
    return processed;
}
