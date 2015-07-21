/** 
 * File:   teovpn.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet VPN application.
 *
 * Created on July 14, 2015, 9:52 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ev_mgr.h"

#define TVPN_VERSION VERSION

#ifdef TEO_THREAD
volatile int teonet_run = 1;
#endif

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

    switch(event) {

        // Calls after event manager stopped
        case EV_K_STOPPED:
            #ifdef TEO_THREAD
            teonet_run = 0;
            #endif
            break;
            
        default:
            break;
    }
}

/*
 * 
 */
int main(int argc, char** argv) {

    printf("Teovpn ver " TVPN_VERSION "\n");
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb /*NULL*/, READ_ALL);
    
    // To run teonet as thread change AM_CONDITIONAL(TEO_THREAD, false) in configure.ac to true
    #ifdef TEO_THREAD
    ksnetEvMgrRunThread(ke);
    for(; teonet_run;) {
        sleep(1);
    }
    #else
    // Start teonet
    ksnetEvMgrRun(ke);
    #endif
    
    return (EXIT_SUCCESS);
}
