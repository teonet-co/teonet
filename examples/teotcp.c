/** 
 * File:   teotcp.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet TCP Client / Server example. 
 * Simple TCP echo server and client connected to it.
 *
 * Created on July 24, 2015, 2:23 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "ev_mgr.h"

#define TTCP_VERSION VERSION    
#define SERVER_PORT 1234
    

/**
 * Teonet Events callback
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {
    
    switch(event) {
        
        // Calls immediately after event manager starts
        case EV_K_STARTED:
        {
            // Type of application (client or server)
//            printf("Type of application: %s\n", 
//                ((ksnetEvMgrAppParam*)user_data)->app_argv_result[1]);
            
            // Start TCP server
            int port_created, 
                fd = ksnTcpServerCreate(ke->kt, SERVER_PORT, NULL, NULL, 
                    &port_created);
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
 * @return
 */
int main(int argc, char** argv) {

    printf("Teotcp ver " TTCP_VERSION "\n");
    
    // Application parameters
    char *app_argv[] = { "", "app_type"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
