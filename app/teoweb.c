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

#include "modules/teo_web/teo_web.h"
#include "modules/teo_web/teo_web_conf.h"
#include "modules/teo_auth/teo_auth.h"

#define TWEB_VERSION "0.0.1"
#define APP_NAME "teoweb"

typedef struct teowebModules {
    
    teoweb_config *tw_cfg;
    
} teowebModules;

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
    
    static ksnHTTPClass *kh = NULL;
    teoweb_config *tw_cfg = ((teowebModules *)ke->user_data)->tw_cfg;

    // Switch Teonet events
    switch(event) {
        
        // Calls immediately after event manager starts
        case EV_K_STARTED:
            
            // Start HTTP server
            kh = ksnHTTPInit(ke, tw_cfg); 
            break;
            
        // Calls before event manager stopped
        case EV_K_STOPPED_BEFORE:
            
            // Stop HTTP server
            if(kh != NULL) ksnHTTPDestroy(kh);
            break;
            
        // Process async messages from teo_web module
        case EV_K_ASYNC:         
            
            if(data != NULL && user_data != NULL) {

                struct mg_connection *nc = user_data;
                struct teoweb_data *td = data;

                // Process websocket events
                switch(td->cmd) {

                    // Web socket client was connected
                    case WS_CONNECTED:
                        
                        ksn_printf(ke, APP_NAME, DEBUG,
                            "async event was received from %p, connected\n", 
                            nc);
                        
                        // Send message to client
                        #define HELLO_MSG "Hello from WS server!"
                        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, 
                                HELLO_MSG, sizeof(HELLO_MSG));
                        
                        break;

                    // Web socket client was disconnected
                    case WS_DISCONNECTED:
                        
                        ksn_printf(ke, APP_NAME, DEBUG,
                            "async event was received from %p, disconnected\n", 
                            nc);
                        break;

                    // Web socket client send a message
                    case WS_MESSAGE: 
                        
                        ksn_printf(ke, APP_NAME, DEBUG,
                            "Async event was received from %p, %d bytes: '%.*s'\n", 
                            nc, (int)data_len, (int)td->data_len, 
                            (char*) td->data);
                        
                        break;
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
    
    printf("Teoweb ver. " TWEB_VERSION ", based on teonet ver. "
            "%s" "\n", teoGetLibteonetVersion());
    
    teowebModules tm;
    
    // Initialize teoweb configuration module
    tm.tw_cfg = teowebConfigInit();
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb, READ_ALL, 0, &tm);
    
    // Set application type
    teoSetAppType(ke, "teo-web");
    teoSetAppVersion(ke, TWEB_VERSION);

    // Read teoweb configuration
    teowebConfigRead(tm.tw_cfg, ke->teo_cfg.network, ke->teo_cfg.port);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    
    // Free teoweb configuration
    teowebConfigFree(tm.tw_cfg);
    
    return (EXIT_SUCCESS);
}
