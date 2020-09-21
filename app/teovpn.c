/** 
 * File:   teovpn.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet VPN application.
 *CMD_USER
 * Created on July 14, 2015, 9:52 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ev_mgr.h"

#include <malloc.h>

#define TVPN_VERSION "0.0.2"

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
        case EV_K_STARTED:
        {
            if(ke->ksn_cfg.l0_public_ipv4[0]) printf("public ipv4: %s\n", ke->ksn_cfg.l0_public_ipv4);
            if(ke->ksn_cfg.l0_public_ipv6[0]) printf("public ipv6: %s\n", ke->ksn_cfg.l0_public_ipv6);
        }
        break;
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

    printf("Teovpn ver " TVPN_VERSION ", based on teonet ver "
            "%s" "\n", teoGetLibteonetVersion());

    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb /*NULL*/, READ_ALL);
    // Set application type
    teoSetAppType(ke, "teo-vpn");
    teoSetAppVersion(ke, TVPN_VERSION);

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
