/* 
 * File:   teoack.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * This Example shows how to get ACK event from remote peer
 *
 * Created on August 14, 2015, 1:53 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TACK_VERSION "0.0.1"    

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
            // TODO: Send message to peer
            break;
            
        // Send by timer
        case EV_K_TIMER:
            {
                static int idx = 0;
                char buffer[KSN_BUFFER_DB_SIZE];

                char *teorecv = "tr-udp-11";
                if(strcmp(teorecv, ksnetEvMgrGetHostName(ke))) {

                    // If teorecv is connected
                    if (ksnetArpGet(ke->kc->ka, teorecv) != NULL) {
                    
                        sprintf(buffer, "#%d: Teoack Hello!", idx++);
                        printf("\nSend message \"%s\" to %s peer\n", buffer, teorecv);
                        ksnCoreSendCmdto(ke->kc, (char*)teorecv, CMD_USER, buffer, 
                                strlen(buffer)+1);

                        ksnetEvMgrSetCustomTimer(ke, 0.00); // Stop timer
                    }
                }
            }
            break;            
            
        // Send when ACK received
        case EV_K_RECEIVED_ACK: 
            {
                // TODO: Got ACK event
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
 * Main application function
 *
 * @param argc
 * @param argv
 * 
 * @return
 */
int main(int argc, char** argv) {

    printf("Teoack example ver " TACK_VERSION ", based on teonet ver. " VERSION "\n");
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION);
    
    // Set custom timer interval
    ksnetEvMgrSetCustomTimer(ke, 1.00);
    
    // Start teonet
    ksnetEvMgrRun(ke);
        
    return (EXIT_SUCCESS);
}
