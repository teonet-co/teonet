/*
 * File:   teonet.c
 * Author: ka_scherba
 *
 * Created on Jun 30, 2015, 10:31:15 AM
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * Simple C Test Suite
 */

#include "ev_mgr.h"

void test1(int argc, char** argv) {
    printf("teonet test 1\n");
    
    printf("Teonet library ver 0.0.1 connection test\n");
    
    // Initialize ksnet event manager and Read configuration (defaults,
    // command line, configuration file)
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, NULL/*event_cb*/, READ_OPTIONS|READ_CONFIGURATION);

    // Hello message
    ksnet_printf(&ke->ksn_cfg, MESSAGE,
            "KSMesh UDP Client Server test ver. 0.0.1\n\n");

    // Start KSNet
    ksnetEvMgrRun(ke);
}

void test2() {
    printf("teonet test 2\n");
    //printf("%%TEST_FAILED%% time=0 testname=test2 (teonet) message=error message sample\n");
}

int main(int argc, char** argv) {
    
    printf("%%SUITE_STARTING%% teonet\n");
    printf("%%SUITE_STARTED%%\n");

    printf("%%TEST_STARTED%% test1 (teonet)\n");
    test1(argc, argv);
    printf("%%TEST_FINISHED%% time=0 test1 (teonet) \n");

    printf("%%TEST_STARTED%% test2 (teonet)\n");
    test2();
    printf("%%TEST_FINISHED%% time=0 test2 (teonet) \n");

    printf("%%SUITE_FINISHED%% time=0\n");

    return (EXIT_SUCCESS);
}
