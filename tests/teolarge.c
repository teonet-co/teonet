/**
 * File:   teolarge.c
 * Author: Kirill Scherba
 *
 * Created on July 18, 2015, 3:06 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TLARGE_VERSION VERSION

/**
 * KSNetwork Events callback
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len) {

    switch(event) {

        // Send by timer
        case EV_K_TIMER:
            {
                char buffer[KSN_BUFFER_SIZE];

                const char *teorecv = "teolrec";
                if(strcmp(teorecv, ksnetEvMgrGetHostName(ke))) {

                    strcpy(buffer, "Large Hello!");
                    ksnCoreSendCmdto(ke->kc, (char*)teorecv, CMD_USER, buffer, KSN_BUFFER_SIZE);
                }
            }
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

        // Undefined event (an error)
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

    printf("Teolarger " TLARGE_VERSION "\n");

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
