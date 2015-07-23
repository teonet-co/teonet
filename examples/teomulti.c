/* 
 * File:   teomulti.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on July 23, 2015, 12:05 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "net_multi.h"

#define TMULTI_VERSION VERSION

#define TEONET_NUM 3
const int TEONET_PORTS[] = { 9301, 9302, 9303 }; // Port numbers
const char *TEONET_NAMES[] = { "TEO-A", "TEO-B", "TEO-C" }; // Hosts names

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {

    printf("Teomulti " TMULTI_VERSION "\n");
    
    ksnMultiData md;
    
    md.argc = argc;
    md.argv = argv;
    md.event_cb = NULL;
    
    md.num = TEONET_NUM;
    md.ports = TEONET_PORTS;
    md.names = TEONET_NAMES;
    
    md.run = 1;
    
    ksnMultiClass *km = ksnMultiInit(&md);
    
    ksnMultiDestroy(km);
    
    return (EXIT_SUCCESS);
}
