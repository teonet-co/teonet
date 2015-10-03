/** 
 * File:   teoweb.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet HTTP/WS server based on Cesanta mongoose: 
 * https://github.com/cesanta/mongoose
 *
 * Created on October 1, 2015, 1:17 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#include "teo_http.h"

#define TWEB_VERSION "0.0.1"

/**
 * Teonet event handler
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 * @param user_data
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {

    // Switch Teonet events
    switch(event) {

        case EV_K_ASYNC:
            {
                struct mg_connection *nc = user_data;
                struct teoweb_data *td = data;
                
                if(data != NULL) {
                    
                    switch(td->cmd) {
                        
                        case WS_CONNECTED:
                            printf("Async event was received from %p, "
                                "connected\n", nc);
                            mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, "Hello!", 6);
                            break;
                            
                        case WS_DISCONNECTED:
                            printf("Async event was received from %p, "
                                "disconnected\n", nc);
                            break;
                            
                        case WS_MESSAGE: 
                            printf("Async event was received from %p, "
                                   "%d bytes: '%.*s'\n", 
                                   nc, (int)data_len, (int)td->data_len, 
                                   (char*) td->data);
                            mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, td->data, td->data_len);
                            break;
                    }
                }                
            }
            break;
            
        default:
            break;
    }
}

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teoweb ver " TWEB_VERSION ", based on teonet ver " VERSION "\n");
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb /*NULL*/, READ_ALL);
    
    // Start HTTP server
    ksnHTTPClass *kh = ksnHTTPInit(ke, 8000, ".");
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    // Stop HTTP server
    ksnHTTPDestroy(kh);

    return (EXIT_SUCCESS);
}
