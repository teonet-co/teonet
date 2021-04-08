/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * teoloadsi application
 *
 * main.c
 * Copyright (C) Kirill Scherba 2017 <kirill@scherba.ru>
 *
 * teoloadsi is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * teoloadsi is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ev_mgr.h"

// Add configuration header
#undef PACKAGE
#undef VERSION
#undef GETTEXT_PACKAGE
#undef PACKAGE_VERSION
#undef PACKAGE_TARNAME
#undef PACKAGE_STRING
#undef PACKAGE_NAME
#undef PACKAGE_BUGREPORT
#include "config.h"

#define APP_VERSION "0.0.1"

#define CMD_T 129
#define INTERVAL 0.25
#define BUFFER_SIZE 128
#define NUM_TO_SHOW 10000
#define SERVER_PEER ke->teo_cfg.app_argv[1]

/**
 * Simple load test data structure
 */
typedef struct cmd_data {
    uint32_t id; ///! Packet id
    uint8_t type; ///! Packet type: 0 - request, 1 - response
    char data[BUFFER_SIZE]; ///! Data buffer
} cmd_data;

// Server flag
static int server_f = 0; ///! Server flag: 1 - this host is a server, 0 - this host is a client

/**
 * Send command with simple load test data
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param to Peer name
 * @param id Packet id
 * @param type Packet type: 0 - request, 1 - response
 */
void sendCmdT(ksnetEvMgrClass *ke, const char* to, int id, int type) {

    // Prepare data
    cmd_data d;
    d.id = id;
    d.type = type;
    strncpy(d.data, "Any commands data", BUFFER_SIZE);

    // Show message
    if(id == 1 || !(id%NUM_TO_SHOW)) ksn_printf(ke, NULL, DEBUG,
            " sendCmdT:      ->> packet #%d ->> '%s'\n", id, to);

    // Send command
    ksnCoreSendCmdto(ke->kc, (char*)to, CMD_T, (void*)&d, sizeof(cmd_data));
}

/**
 * Teonet event handler
 *
 * @param ke
 * @param event
 * @param data
 * @param data_length
 * @param user_data
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_length, void *user_data) {

    static int id = 0;
    static int errors = 0;
    static int start_f = 1;
    const ksnCorePacketData *rd = (ksnCorePacketData *) data;

    switch(event) {

        // Start sending by first timer
        case EV_K_TIMER:
            // Is it server
            if(start_f) {
                if(!strcmp(SERVER_PEER, "none")) server_f = 1;

                if(!server_f) {
                    ksn_printf(ke, "", DEBUG, "Client mode, server name: %s\n\n", SERVER_PEER);
                    id = 0;
                    sendCmdT(ke, SERVER_PEER, ++id, 0);
                }
                else ksn_puts(ke, "", DEBUG, "Server mode\n");

                start_f = 0;
            }
            break;

        // When peer connected
        case EV_K_CONNECTED:
            ksn_printf(ke, "", DEBUG, "EV_K_CONNECTED: %s\n\n", rd->from);
            break;

        // When command received
        case EV_K_RECEIVED:
            switch(rd->cmd) {
                case CMD_T: {
                    const cmd_data *d = rd->data;

                    // Answer received with correct id
                    if(d->type == 1 && d->id == id) {
                        if(id == 1 || !(id%NUM_TO_SHOW)) {
                            ksn_printf(ke, "", DEBUG,
                                "EV_K_RECEIVED: <<- packet #%d <<- '%s'\n",
                                d->id, rd->from
                            );
                            if(errors)
                                ksn_printf(ke, "", DEBUG, ", %d - errors",
                                        errors);
                            printf("\n");
                        }
                    }

                    // Request received
                    else if(d->type == 0) {
                        if(!(d->id%NUM_TO_SHOW))
                            ksn_printf(ke, "", DEBUG,
                                "EV_K_RECEIVED: <<- packet #%d <<- '%s'\n",
                                d->id, rd->from);
                    }

                    // Answer received with incorrect id
                    else {
                        errors++;
                        ksn_printf(ke, "", ERROR_M,
                               "EV_K_RECEIVED: <<- packet #%d <<- '%s'"
                               " - Incorrect packet ID\n",
                               d->id, rd->from);
                    }

                    // Send answer or new packet
                    if(d->type == 0) sendCmdT(ke, rd->from, d->id, 1);
                    else if(!server_f) sendCmdT(ke, SERVER_PEER, ++id, 0);

                } break;
            }
            break;

        // Calls after event manager stopped
        case EV_K_STOPPED:
            break;

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

    printf("Teonet simple load test C++ application ver " APP_VERSION ", "
           "based on teonet ver. %s\n",
           teoGetLibteonetVersion()
    );

    // Application parameters
    const char *app_argv[] = { "", "server_peer" };
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    app_param.app_descr = NULL;

    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);

    // Set application type
    teoSetAppType(ke, "teoloadsi");

    // Set application version
    teoSetAppVersion(ke, APP_VERSION);

    // Start Timer event
    ksnetEvMgrSetCustomTimer(ke, INTERVAL);

    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
