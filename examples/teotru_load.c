/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 * \file   teotru_load.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * TR-UDP loading test
 *
 * Created on April 4, 2016, 3:54 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TUDPL_VERSION "0.0.2"

int start_test_01 = 0;
int app_mode = 0; // 0 -client mode; 1- server mode

void test_01(ksnetEvMgrClass *ke, char *server_peer) {

    static int num = 0;
    const size_t BUF_LEN = 1024;
    const size_t NUM_RECORDS = 1024;

    printf("\nTest 01 #%d: ", ++num);
    printf("Send %d records of %d bytes each messages ... ", (int)NUM_RECORDS, (int)BUF_LEN);
    fflush(stdout);

    ksnCoreSendCmdto(ke->kc, server_peer, CMD_USER + 1, "Test 01", 8);

    char buffer[BUF_LEN];
    for(int i = 0; i < NUM_RECORDS; i++) {
        snprintf(buffer, BUF_LEN, "%d", i);
        ksnCoreSendCmdto(ke->kc, server_peer, CMD_USER, buffer,
                            BUF_LEN /*strlen(buffer)+1*/);
    }

    ksnCoreSendCmdto(ke->kc, server_peer, CMD_USER + 2, "Test 01", 8);
    printf("done\n");
    start_test_01 = 1;
}

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

        case EV_K_STARTED:
        {
            // Server mode
            if(!ke->ksn_cfg.app_argv[1][0] || !strcmp(ke->ksn_cfg.app_argv[1], "server")) {

                printf("Server mode application started\n");
                printf("Wait for client connected ...\n");
                app_mode = 1;
            }

            // Client mode
            else {

                printf("Client mode application started ...\n");
                printf("Wait connection to server \"%s\" ...\n", ke->ksn_cfg.app_argv[1]);
                app_mode = 0;
            }

        } break;

        // Send by timer
        case EV_K_TIMER:
        {
            // Client mode
            if(app_mode == 0) {
                if(start_test_01) {
                    start_test_01 = 0;
                    test_01(ke, ke->ksn_cfg.app_argv[1]);
                }
            }
        } break;

        case EV_K_CONNECTED:
        {
            // Client / Server mode
            ksnCorePacketData *rd = data;
            printf("Peer %s connected at %s:%d \n",
                    rd->from, rd->addr, rd->port);

            // Client mode
            if(!strcmp(rd->from, ke->ksn_cfg.app_argv[1])) {
                printf("Connected to server \"%s\" ...\n\n"
                       "Press A to start/stop \"Test 01\"\n\n",
                        ke->ksn_cfg.app_argv[1]);
            }

        } break;

        // Data received event
        case EV_K_RECEIVED:
        {
            static int num = 0;
            static int num_data_cmds = 0;
            ksnCorePacketData *rd = (ksnCorePacketData *)data;

            // Command processing
            switch(rd->cmd) {

                case CMD_USER + 1:
                    // Server mode
                    num_data_cmds = 0;
                    printf("\nTest \"%s\" #%d from %s started\n",
                            (char*)rd->data, ++num, rd->from);
                    break;

                case CMD_USER:
                    // Server mode
                    if(num_data_cmds != atoi((char*)rd->data))
                        printf("\rError: %s", (char*)rd->data);
                    else
                        printf("\rGot data: %s", (char*)rd->data);
                    fflush(stdout);

                    num_data_cmds++;
                    break;

                case CMD_USER + 2:
                    // Server mode
                    printf("\rGot %d commands\nTest \"%s\" #%d from %s stopped\n",
                            num_data_cmds, (char*)rd->data, num, rd->from);
                    num_data_cmds = 0;
                    break;
            }
        } break;

        // Teo ACK received use ksnetAllowAckEvent to allow this event
        case EV_K_RECEIVED_ACK:
        {
            // Client mode
            printf("*");
            fflush(stdout);
        } break;

        // 'A' key was pressed in application hotkey monitor
        case EV_K_USER:
        {
            // Client mode
            if(app_mode == 0) start_test_01 = !start_test_01 ? 1:0;
        } break;

        default:
            break;
    }
}

/**
 * Main teotru_load application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 *
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

    // Label
    printf("Teonet TR-UDP loading test ver " TUDPL_VERSION ", "
           "based on teonet ver. " VERSION "\n");

    // Application parameters
    const char *app_argv[] = { "", "server_peer"};
    const char *app_argv_descr[] = { "", "\"Server peer name to connect this host to\" in client mode; or \"\" or \"server\" in server mode" };
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    app_param.app_descr = app_argv_descr;

    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);

    // Set application type
    teoSetAppType(ke, "teo-trudp-loading");
    teoSetAppVersion(ke, TUDPL_VERSION);

    // Set custom timer interval
    ksnetEvMgrSetCustomTimer(ke, 2.00);

    // Allow ACK event
    //ksnetAllowAckEvent(ke, 1);

    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
