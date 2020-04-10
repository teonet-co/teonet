/** 
 * \file   teogw.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * 
 * ## Connect to multi networks with one call
 * 
 * Connect to and manage some teo-networks in at one time (without using threads). 
 * The networks are divided by the host port number and (or) network name.
 * Use config file to connect networks to it r-hosts.
 *
 * Created on Mart 27, 2015, 19:51 PM
 * Updated on September 27, 2017, 21:41 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "net_multi.h"

#define TGW_VERSION "0.0.2"

#define TEONET_NUM 2
const int TEONET_PORTS[] = { 9040, 9042 }; // Port numbers
const char *TEONET_NAMES[] = { "teo-gw", "teo-gw-local" }; // Hosts names
const char *TEONET_NETWORKS[] = { "local", "teonet" }; // Networks


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

    // Switch Teonet event
    switch(event) {
        
        // Set default namespace
        case EV_K_STARTED:
            
            ksn_printf(ke, NULL, DEBUG, "Host '%s' started at network '%s'...\n", 
                    ksnetEvMgrGetHostName(ke), ke->ksn_cfg.network);
                    
            // Set application type
            teoSetAppType(ke, "teo-gw");
            teoSetAppVersion(ke, TGW_VERSION);

            // start new network
            if(!strcmp(ke->ksn_cfg.network,"local")) {
                const char* net = "NEW_NET";
                ksn_printf(ke, NULL, DEBUG, "Dynamically add new network %s\n", net);
                teoMultiAddNet(ke->km, event_cb, "NEW_HOST", 0, net);
            }

            break;
            
        default:
            break;
    }
}

/**
 * Main application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 * 
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

    printf("Teonet gateway ver " TGW_VERSION ", based on teonet ver "
            "%s" "\n", teoGetLibteonetVersion());
    
    ksnMultiData md;
    
    md.argc = argc;
    md.argv = argv;
    md.event_cb = event_cb;
    
    md.num = TEONET_NUM;
    md.ports = TEONET_PORTS;
    md.names = TEONET_NAMES;
    md.networks = TEONET_NETWORKS;
    
    md.run = 1;
    
    ksnMultiClass *km = ksnMultiInit(&md, NULL);
    
    ksnMultiDestroy(km);
    
    return (EXIT_SUCCESS);
}
