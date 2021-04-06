/**
 * \file   teosend.c
 * \author Kirill Scherba <kirill@scherba.ru>  
 *
 * Created on July 15, 2015, 5:48 PM
 * 
 * \example teosend.c
 *
 * ## Send and receive teonet messages
 *
 * Test application to send and receive teonet messages.
 * 
 * How to execute this test:
 * 
 * 1) Start one teonet test application in terminal:
 * 
 *      tests/teosend teosend -p 9500
 * 
 * 2) Start another test application in other terminal:
 * 
 *      tests/teosend teorecv -r 9500 -a 127.0.0.1
 *
 * - subscribe to timer event
 * - send message by timer
 * - show received messages
 * - show idle event
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TSEND_VERSION VERSION

/**
 * KSNetwork Events callback
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
            {
                ksnCorePacketData *rd = data;
                printf("Event: Data received %d bytes\n", (int)data_len);
                //host_received_cb(ke, data, data_len);
                printf("Command: %d, Data: %s, got from: %s\n", rd->cmd, (char*)rd->data, rd->from);
            }
            break;

        // Send when idle (every 11.5 sec idle time)
        case EV_K_IDLE:
            printf("Event: Idle time\n");
            //host_idle_cb(ke);
            break;

        // Send by timer
        case EV_K_TIMER:
            {
                const char *teorecv = "teorecv";
                if(strcmp(teorecv, ksnetEvMgrGetHostName(ke))) {
                    ksnCoreSendCmdto(ke->kc, (char*)teorecv, CMD_USER, "Hello!", 7);
                }
            }
            break;

        // Undefined event (an error)
        default:
            break;
    }
}

/**
 * Main Teosend application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 * 
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

    printf("Teosend ver " TSEND_VERSION "\n");

    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION);

    // Set custom timer interval
    ksnetEvMgrSetCustomTimer(ke, 1.00);

    // Hello message
    ksnet_printf(&ke->teo_cfg, MESSAGE, "Started...\n\n");

    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
