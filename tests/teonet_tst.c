/** 
 * \file   teonet_tst.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Simple C Test Suite
 * 
 * Simple C test code: \include teonet_tst.c
 * 
 * Created on Jun 30, 2015, 10:31:15 AM
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * Simple C Test Suite
 */

#include "ev_mgr.h"

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

        // Send immediately after event manager starts
        case EV_K_STARTED:
            printf("Event: Event manager started\n");
            //host_started_cb(ke);
            break;

        // Send when new peer connected to this host (and to the mesh)
        case EV_K_CONNECTED:
            printf("Event: Peer '%s' was connected\n", ((ksnCorePacketData*)data)->from);
            break;

        case EV_K_DISCONNECTED:
            printf("Event: Peer '%s' was disconnected\n", ((ksnCorePacketData*)data)->from);
            break;

        // Send when data received
        case EV_K_RECEIVED:
            printf("Event: Data received\n");
            //host_received_cb(ke, data, data_len);
            break;

        // Send when idle (every 11.5 sec idle time)
        case EV_K_IDLE:
            printf("Event: Idle time\n");
            //host_idle_cb(ke);
            break;

        // Undefined event (an error)
        default:
            break;
    }
}

void test_2_1() {
    printf("teonet test 1\n");
    //printf("%%TEST_FAILED%% time=0 testname=test1 (teonet) message=error message sample\n");    
}

void test2(int argc, char** argv) {
    printf("teonet test 2\n");
    
    printf("Teonet library ver " VERSION " connection test\n");
    
    // Initialize teonet event manager and Read configuration (defaults,
    // command line, configuration file)
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb /* NULL */, READ_OPTIONS|READ_CONFIGURATION);

    // Hello message
    ksnet_printf(&ke->ksn_cfg, DISPLAY_M,
            "KSMesh UDP Client Server test ver. " VERSION "\n\n");

    // Start teonet
    ksnetEvMgrRun(ke);
}

int main(int argc, char** argv) {
    
    printf("%%SUITE_STARTING%% teonet\n");
    printf("%%SUITE_STARTED%%\n");

    printf("%%TEST_STARTED%% test1 (teonet)\n");
    test_2_1();
    printf("%%TEST_FINISHED%% time=0 test1 (teonet) \n");

    printf("%%TEST_STARTED%% test2 (teonet)\n");
    test2(argc, argv);
    printf("%%TEST_FINISHED%% time=0 test2 (teonet) \n");

    printf("%%SUITE_FINISHED%% time=0\n");

    return (EXIT_SUCCESS);
}
