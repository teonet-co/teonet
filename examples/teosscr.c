/** 
 * \file   teosscr.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * \example   teosscr.c
 * 
 * ## Using the Teonet Subscribe module
 * Module: subscribe.c  
 *   
 * This is example application to subscribe one teonet application to other 
 * teonet application event. 
 * 
 * In this example we:
 * * start one application as server: 
 *   ```examples/teosscr teosscr-ser null```
 * * start other application as client: 
 *   ```examples/teosscr teosscr-cli teosscr-ser -r 9000 -a 127.0.0.1```
 * 
 * Created on January 5, 2016, 3:38 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TSSCR_VERSION "0.0.1"

/**
 * This Application type:
 *  0 - client
 *  1 - server
 */
static int app_type = 0;

/**
 * Clients Teonet Events callback
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param event Teonet event selected in ::ksnetEvMgrEvents enum
 * @param data Events data
 * @param data_len Data length
 * @param user_data User data selected in ksnetEvMgrInitPort() function
 */
void event_cb_client(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {
    
    switch(event) {
    
        // Connected event received
        case EV_K_CONNECTED:
        {
            // Client send subscribe command to server                
            char *peer = ((ksnCorePacketData*)data)->from;
            if(!strcmp(peer, ke->ksn_cfg.app_argv[1])) {

                printf("The peer: '%s' was connected\n", peer);

                // Subscribe to EV_K_RECEIVED event in remote peer
                teoSScrSubscribe(ke->kc->kco->ksscr, peer, EV_K_RECEIVED);
            }
        }
        break;
        
        // Calls by timer started in main() function by calling the 
        // ksnetEvMgrSetCustomTimer() function
        case EV_K_TIMER:
        {
            // Client send CMD_USER command to server
            ksnCoreSendCmdto(ke->kc, ke->ksn_cfg.app_argv[1], CMD_USER, 
                        "Hello!", 7);
        }
        break;
        
        // Subscribe event received
        case EV_K_SUBSCRIBE:
        {
            ksnCorePacketData *rd = data;
            teoSScrData *ssrc_data = rd->data;
            
            printf("EV_K_SUBSCRIBE received from: %s, event: %d, command: %d, "
                   "from: %s, data: %s\n", 
                    rd->from, ssrc_data->ev, ssrc_data->cmd, ssrc_data->data, 
                    ssrc_data->data + strlen(ssrc_data->data) + 1 );
        }
        break;        
        
        // Sum other events
        default:
            break;
    }    
}
    
/**
 * Servers Teonet Events callback
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param event Teonet event selected in ::ksnetEvMgrEvents enum
 * @param data Events data
 * @param data_len Data length
 * @param user_data User data selected in ksnetEvMgrInitPort() function
 */
void event_cb_server(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {
    
    switch(event) {
    
        // Calls when client peer disconnected
        case EV_K_DISCONNECTED:
        {
            ksnCorePacketData *rd = data;
            if(rd->from != NULL) {
                
                printf("Peer %s was disconnected \n", rd->from);
                
                // Server UnSubscribe peer
                // UnSubscribe peer from EV_K_RECEIVED event
                teoSScrUnSubscription(ke->kc->kco->ksscr, rd->from,
                        EV_K_RECEIVED);
            }
            else printf("Peer was disconnected\n");
        }    
        break;
        
        // Calls when data received
        case EV_K_RECEIVED:
        {
            // Server send EV_K_SUBSCRIBE command to client when CMD_USER 
            // command received
            ksnCorePacketData *rd = data;
            if(rd->cmd == CMD_USER) {

                printf("EV_K_RECEIVED command: %d, from %s, data: %s\n", 
                        rd->cmd, rd->from, (char*)rd->data);

                size_t rec_data_len = rd->from_len + rd->data_len;
                char *rec_data = malloc(rec_data_len);
                memcpy(rec_data, rd->from, rd->from_len);
                memcpy(rec_data + rd->from_len, rd->data, rd->data_len);
                
                // Send EV_K_RECEIVED event to all subscribers
                teoSScrSend(ke->kc->kco->ksscr, EV_K_RECEIVED, rec_data, 
                        rec_data_len, rd->cmd);
                
                free(rec_data);
            }
        }    
        break;
        
        // Sum other events
        default:
            break;
    }
}

/**
 * Common Teonet Events callback
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param event Teonet event selected in ::ksnetEvMgrEvents enum
 * @param data Events data
 * @param data_len Data length
 * @param user_data User data selected in ksnetEvMgrInitPort() function
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {
    
    switch(event) {
        
        // Calls immediately after event manager starts
        case EV_K_STARTED:
        {
            // Type of application (client or server)
            printf("Type of this application: "); 
            
            // Client
            if(strcmp(ke->ksn_cfg.app_argv[1], "null")) {
                
                printf("Client peer subscribed to '%s' peer\n", 
                        ke->ksn_cfg.app_argv[1]);       
                
                app_type = 0;
            }
            
            // Server
            else {
                
                printf("Server peer to subscribe to\n");
                
                app_type = 1;
            }
        }
        break;
                    
        // All other events (switch to clients or servers event_cb)
        default:
            if(!app_type) event_cb_client(ke, event, data, data_len, user_data);
            else  event_cb_server(ke, event, data, data_len, user_data);
            break;
    }
}

/**
 * Main Subscribe example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 * 
 * @return
 */
int main(int argc, char** argv) {
    
    // Show application info
    printf("Teosscr example ver " TSSCR_VERSION ", "
           "based on teonet ver. " VERSION "\n");

    // Application parameters
    char *app_argv[] = { "", "remote_peer"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Set custom timer interval. See "case EV_K_TIMER" to continue this example
    ksnetEvMgrSetCustomTimer(ke, 2.00);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    return (EXIT_SUCCESS);
}
