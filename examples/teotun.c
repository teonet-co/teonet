/** 
 * File:   teotun.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teotun.c
 * 
 * ## Teonet TCP Tunnel example
 * 
 * To execute this example start teotun peer. The example will connect to it. So 
 * start teonet application with teotun name, for example:
 * 
 * ~~~~~~~~~~~~~~~{.sh}
 * app/teovpn teotun
 * ~~~~~~~~~~~~~~~
 * 
 * The start examples/teotun application to create tunnel to port 22 at the 
 * teotun peer. This tunnel will open port 7522 at host where teotun example 
 * started:
 * 
 * ~~~~~~~~~~~~~~~{.sh}  
 * examples/teotun teotun_c
 * ~~~~~~~~~~~~~~~
 * 
 * To test created tunnel use ssh and connect to localhost at port 7522:
 * 
 * ~~~~~~~~~~~~~~~{.sh} 
 * ssh user@localhost -p 7722
 * ~~~~~~~~~~~~~~~
 * 
 * (_replace user with valid user name at remote teotun peer_)
 *
 * Created on July 27, 2015, 11:26 AM
 */

#include <stdio.h>
#include <stdlib.h>

#include "ev_mgr.h"

#define TTUN_VERSION VERSION

#define TUN_PEER "teotun"
#define TUN_PEER_PORT 22
#define TUN_PORT 7522

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
        {
            // Tunnel parameters message
            printf("The tunnel at port %d to port %d at %s peer was created\n",  
                    TUN_PORT, TUN_PEER_PORT, TUN_PEER);
            
            // Create tunnel
            ksnTunCreate(ke->ktun, TUN_PORT, TUN_PEER, TUN_PEER_PORT, NULL);
        }
        break;
        
        default:
            break;
    }
}

/**
 * Main Teotun application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 * 
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

    printf("Teotun example ver " TTUN_VERSION "\n");
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    return (EXIT_SUCCESS);
}
