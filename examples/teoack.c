/** 
 * \file   teoack.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teoack.c
 * 
 * ## Get ACK event
 * 
 * This Example shows how to get ACK event from remote peer
 *
 * Created on August 14, 2015, 1:53 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TACK_VERSION "0.0.2"    

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
        case EV_K_TIMER:
        {
            static int idx = 0;
            char buffer[KSN_BUFFER_DB_SIZE];

            char *peer_to = ke->teo_cfg.app_argv[1]; //"tr-udp-11";
            if(strcmp(peer_to, ksnetEvMgrGetHostName(ke))) {

                // If peer_to is connected
                if (ksnetArpGet(ke->kc->ka, peer_to) != NULL) {

                    sprintf(buffer, "#%d: Teoack Hello!", idx++);
                    printf("\nSend message \"%s\" to %s peer\n", buffer, peer_to);
                    ksnCoreSendCmdto(ke->kc, (char*)peer_to, CMD_USER, buffer, 
                            strlen(buffer)+1);

                    ksnetEvMgrSetCustomTimer(ke, 0.00); // Stop timer
                }
            }
        }
        break;            
            
        // Teo ACK received use ksnetAllowAckEvent to allow this event
        case EV_K_RECEIVED_ACK: 
        {
            // Got ACK event
            ksnCorePacketData *rd = data;
            printf("Got ACK event to ID %d, data: %s\n", 
                   *(uint32_t*)user_data, (char*)rd->data);


            ksnetEvMgrSetCustomTimer(ke, 1.00); // Set custom timer interval
        }
        break;
            
        // Undefined event (an error)
        default:
            break;
    }
}            

/**
 * Main Teoack application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 * 
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

    printf("Teoack example ver " TACK_VERSION ", based on teonet ver. " VERSION "\n");
    
    
    // Application parameters
    const char *app_argv[] = { "", "peer_to"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    app_param.app_descr = NULL;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Set custom timer interval
    ksnetEvMgrSetCustomTimer(ke, 1.00);
    
    // Allow ACK event
    ksnetAllowAckEvent(ke, 1);
    
    // Start teonet
    ksnetEvMgrRun(ke);
        
    return (EXIT_SUCCESS);
}
