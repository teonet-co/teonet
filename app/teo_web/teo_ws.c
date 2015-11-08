/** 
 * File:   teo_ws.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet websocket L0 connector module
 *
 * Created on November 8, 2015, 3:58 PM
 */

#include <stdlib.h>
#include <stdio.h>
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
    
    printf("Initialize\n");
    
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
    
    printf("Destroy\n");
    
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
            
            processed = kws->processMsg(kws, nc_p, data, data_length);
                    
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
    const char *login = "login, ";
    const size_t login_len = strlen(login);
    
    if(data_length >= login_len &&
       !strncmp(data, login, login_len)) {
        
        printf("Login received\n");
        processed = 1;
    }
    
    // Check for L0 websocket Peer command
    const char *peer = "peer, ";
    const size_t peer_len = strlen(peer);
    
    if(data_length >= peer_len &&
       !strncmp(data, peer, peer_len)) {
        
        printf("Peer request command received\n");
        processed = 1;
    }
    
    
    // Check for L0 websocket ECHO command
    const char *echo = "echo, ";
    const size_t echo_len = strlen(echo);
    
    if(data_length >= echo_len &&
       !strncmp(data, echo, echo_len)) {
        
        printf("Echo command received\n");
        processed = 1;
    }
    
    return processed;
}
