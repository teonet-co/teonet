/** 
 * \file   teogw.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * 
 * ## Connect to multi networks with one call
 * 
 * Connect to and manage some teo-networks in one time (without using threads). 
 * The networks are divided by the host port number.
 *
 * Created on Mart 27, 2015, 19:51 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "net_multi.h"

#define TGW_VERSION "0.0.1"

#define TEONET_NUM 2
const int TEONET_PORTS[] = { 9040, 9042 }; // Port numbers
const char *TEONET_NAMES[] = { "teo-gw", "teo-gw-main" }; // Hosts names

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
    md.event_cb = NULL;
    
    md.num = TEONET_NUM;
    md.ports = TEONET_PORTS;
    md.names = TEONET_NAMES;
    
    md.run = 1;
    
    ksnMultiClass *km = ksnMultiInit(&md);
    
    // Set application type
//    teoSetAppType(ke, "teo-gw");
//    teoSetAppVersion(ke, TDB_VERSION);
    
    ksnMultiDestroy(km);
    
    return (EXIT_SUCCESS);
}
