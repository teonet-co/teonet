/**
 * File:   teolarge.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teolarge.c
 * 
 * ## Send and receive large data blocks between teonet applications
 * 
 * How to execute this test:
 * 
 * 1) Start one teonet test application in terminal:
 * 
 *      examples/teolarge teolarge -p 9500
 * 
 * 2) Start another test application in other terminal:
 * 
 *      examples/teolarge teorecv -r 9500 -a 127.0.0.1
 * 
 * The first application with name teolarge will send data blocks 2048 bytes 
 * length to the teorecv application.
 *
 * Created on July 18, 2015, 3:06 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TLARGE_VERSION VERSION

/**
 * Teonet Events callback
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

        // Send by timer
        case EV_K_TIMER:
            {
                char buffer[KSN_BUFFER_DB_SIZE];

                const char *teorecv = "teorecv";
                if(strcmp(teorecv, ksnetEvMgrGetHostName(ke))) {

                    strcpy(buffer, "Large Hello!");
                    ksnCoreSendCmdto(ke->kc, (char*)teorecv, CMD_USER, buffer, 
                            KSN_BUFFER_DB_SIZE);
                }
            }
            break;

        // Send when data received
        case EV_K_RECEIVED:
            {
                ksnCorePacketData *rd = data;
                printf("Command: %d, Data: %s (%d bytes), got from: %s\n", 
                        rd->cmd, (char*)rd->data, (int)rd->data_len, rd->from);
            }
            break;

        // Undefined event (an error)
        default:
            break;
    }
}

/**
 * Main Teolarge application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 * 
 * @return
 */
int main(int argc, char** argv) {

    printf("Teolarge " TLARGE_VERSION "\n");

    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION);

    // Set custom timer interval
    ksnetEvMgrSetCustomTimer(ke, 1.00);

    // Hello message
    ksnet_printf(&ke->ksn_cfg, MESSAGE, "Started...\n\n");

    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
