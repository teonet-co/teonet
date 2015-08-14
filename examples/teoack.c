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
                char buffer[KSN_BUFFER_DB_SIZE];

                const char *teorecv = "tr-udp-11";
                if(strcmp(teorecv, ksnetEvMgrGetHostName(ke))) {

                    strcpy(buffer, "Teoack Hello!");
                    printf("Send message \"%s\" to %s peer\n", buffer, teorecv);
                    ksnCoreSendCmdto(ke->kc, (char*)teorecv, CMD_USER, buffer, 
                            strlen(buffer)+1);
                }
            }
            break;            
            
        // Send when ACK received
        case EV_K_RECEIVED_ACK:
            // TODO: Got ACK event
            printf("Got ACK event to ID %d, data: %s\n", *(uint32_t*)user_data, (char*)data);
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
