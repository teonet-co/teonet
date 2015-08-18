/* 
 * File:   teoackm.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * This Example shows how to send multi records with TR-UDP and get ACK event 
 * from remote peer
 *
 * Created on August 14, 2015, 1:53 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TACKM_VERSION "0.0.1"    
//#define NUM_PACKET 200

/**
 * Teonet Events callback
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 * @param user_data
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {
    
    switch(event) {
        
        // Calls immediately after event manager starts
        case EV_K_STARTED:
            break;
            
        // Send by timer
        case EV_K_USER:    
        case EV_K_TIMER:
        {
            static int i, idx = 0;
            char buffer[KSN_BUFFER_DB_SIZE];

            char *peer_to = ke->ksn_cfg.app_argv[1]; 
            if(strcmp(peer_to, ksnetEvMgrGetHostName(ke))) {

                // If peer_to is connected
                if (ksnetArpGet(ke->kc->ka, peer_to) != NULL) {

                    int NUM_PACKET;
                    
                    ksnetEvMgrSetCustomTimer(ke, 0.00); // Stop timer
                    
                    //for(;;) {
                        
                    printf("How much package to send (0 - to exit): ");
                    _keys_non_blocking_stop(ke->kh);
                    scanf("%d", &NUM_PACKET);
                    _keys_non_blocking_start(ke->kh);

                    // Exit
                    if(NUM_PACKET <= 0) {

                        ev_break (ke->ev_loop, EVBREAK_ONE);
                        break;
                    }
                    // Send packages
                    else {

                        printf("Send %d messages to %s\n", 
                                NUM_PACKET, peer_to);

                        for(i = 0; i < NUM_PACKET; i++) {

                            sprintf(buffer, "#%d: Teoack Hello!", idx++);
                            printf("Send message \"%s\"\n", buffer);
                            ksnCoreSendCmdto(ke->kc, (char*)peer_to, 
                                CMD_USER, buffer, strlen(buffer)+1);
                        }
                    }
                    printf("Press U to repeat\n");
                }
            }
        }
        break;            
        
        // Send when DATA received
        case EV_K_RECEIVED:
        {
            // DATA event
            ksnCorePacketData *rd = data;
            printf("Got DATA ID %d: %s\n", 
                   *(uint32_t*)user_data, (char*)rd->data); 
        }
        break;
            
        // Send when ACK received
        case EV_K_RECEIVED_ACK: 
        {
            // ACK event
            ksnCorePacketData *rd = data;
            printf("Got ACK event to ID %d, data: %s\n", 
                   *(uint32_t*)user_data, (char*)rd->data);

            //ksnetEvMgrSetCustomTimer(ke, 1.00); // Set custom timer interval
        }
        break;
            
        // Undefined event (an error)
        default:
            break;
    }
}            

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * 
 * @return
 */
int main(int argc, char** argv) {

    printf("Teoack multi example ver " TACKM_VERSION ", based on teonet ver. "
            VERSION "\n");
    
    
    // Application parameters
    char *app_argv[] = { "", "peer_to"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Set custom timer interval
    ksnetEvMgrSetCustomTimer(ke, 1.00);
    
    // Start teonet
    ksnetEvMgrRun(ke);
        
    return (EXIT_SUCCESS);
}
