/* 
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

#include "ev_mgr.h"

#define TWEB_VERSION "0.0.1"

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

    // Switch Teonet events
    switch(event) {

        default:
            break;
    }
}

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teoweb ver " TWEB_VERSION ", based on teonet ver "
            VERSION "\n");
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb /*NULL*/, READ_ALL);
    
    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
