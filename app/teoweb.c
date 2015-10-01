/** 
 * File:   teoweb.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet HTTP/WS server based on Cesanta mongoose: 
 * https://github.com/cesanta/mongoose
 *
 * Created on October 1, 2015, 1:17 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#include "teo_http.h"

#define TWEB_VERSION "0.0.1"

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teoweb ver " TWEB_VERSION ", based on teonet ver " VERSION "\n");
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, ksnHTTPEventCb /*NULL*/, READ_ALL);
    
    // Start HTTP server
    ksnHTTPClass *kh = ksnHTTPInit(ke);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    // Stop HTTP server
    ksnHTTPDestroy(kh);

    return (EXIT_SUCCESS);
}
