/**
 * \file   teostream.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teostream.c
 *
 * Teonet stream example
 * 
 * Created on October 5, 2015, 5:06 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TSTR_VERSION "0.0.1"

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
        
        // Send when new peer connected to this host (and to the mesh)
        case EV_K_CONNECTED:            
            if(data != NULL) {
                
                if(!strcmp(((ksnCorePacketData*)data)->from, ke->ksn_cfg.app_argv[1])) {
                    
                    printf("Peer '%s' was connected\n", ((ksnCorePacketData*)data)->from);

                    printf("Create stream name \"%s\" with peer \"%s\" ...\n",  
                        ke->ksn_cfg.app_argv[2], ke->ksn_cfg.app_argv[1]); 

                    ksnStreamCreate(ke->ks, ke->ksn_cfg.app_argv[1], 
                        ke->ksn_cfg.app_argv[2],  CMD_ST_CREATE);
                }
            
            }
            break;
            
        // Send when stream is connected to this host 
        case EV_K_STREAM_CONNECTED:            
            printf("Stream name \"%s\" is connected to peer \"%s\" ...\n",  
                        (char*)data + strlen(data) + 1, (char*)data); 
            break;

        // Undefined event (an error)
        default:
            break;
    }
}

/**
 * Main Teostream application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 * 
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teostream example ver " TSTR_VERSION ", based on teonet ver. " VERSION "\n");
    
    // Application parameters
    char *app_argv[] = { "", "peer_to", "stream_name"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 3;
    app_param.app_argv = app_argv;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
