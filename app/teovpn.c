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

#include "ev_mgr.h"

#define TVPN_VERSION VERSION

/*
 * 
 */
int main(int argc, char** argv) {

    printf("Teovpn ver " TVPN_VERSION "\n");
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, NULL, READ_ALL);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    return (EXIT_SUCCESS);
}
