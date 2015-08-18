/* 
 * File:   teoackm.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * This Example shows how to send multi records with TR-UDP and get ACK event 
 * from remote peer
 *
 * Created on August 14, 2015, 1:53 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "net_tr-udp_stat.h"

#define TACKM_VERSION "0.0.1"    

// This application commands
#define CMD_U_STAT  "stat"
#define CMD_U_RESET "stat"

#define SERVER_NAME "none"

const char* PRESS_U = "(Press U to return to main menu)";

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
    
    char *peer_to = ke->ksn_cfg.app_argv[1]; 
    
    switch(event) {
        
        // Calls immediately after event manager starts
        case EV_K_STARTED:
            if(strcmp(peer_to, SERVER_NAME))
                printf("Connecting to peer: %s ...\n", peer_to);
            else
                printf("Server mode\n");
            break;
            
        // Send when peer connected
        case EV_K_CONNECTED:
        {
            ksnCorePacketData *rd = data;
            if(!strcmp(rd->from, peer_to))
                printf("Peer %s connected at %s:%d \n", 
                       rd->from, rd->addr, rd->port);
        }    
        break;
            
        // Send when peer disconnected
        case EV_K_DISCONNECTED: 
        {
            printf("disconnected\n");
            ksnCorePacketData *rd = data;
            if(rd->from != NULL)
                if(!strcmp(rd->from, peer_to))
                    printf("Peer %s was disconnected \n", 
                rd->from);
        }    
        break;
            
        // Send by timer
        case EV_K_USER:    
        case EV_K_TIMER:
        {
            static int i, idx = 0;
            char buffer[KSN_BUFFER_DB_SIZE];

            if(strcmp(peer_to, ksnetEvMgrGetHostName(ke))) {

                // If peer_to is connected
                if (ksnetArpGet(ke->kc->ka, peer_to) != NULL) {

                    int num_packets, command;
                    
                    ksnetEvMgrSetCustomTimer(ke, 0.00); // Stop timer
                    
                    for(;;) {
                        
                        printf("\n"
                               "Multi send test menu:\n"
                               "\n"
                               "  1 - send packets\n"
                               "  2 - show TR-UDP statistics\n" 
                               "  3 - get and show remote peer TR-UDP statistics\n" 
                               "  4 - send TR-UDP reset\n" 
                               "  0 - exit\n"
                               "\n"
                               "teoackm $ "
                        );

                        // Get command
                        _keys_non_blocking_stop(ke->kh);
                        scanf("%d", &command);
                        _keys_non_blocking_start(ke->kh);
                        
                        // Check command
                        switch(command) {

                            // Exit
                            case 0:
                                break;

                            // Send packets
                            case 1:
                                printf("How much package to send (0 - to exit): ");
                                _keys_non_blocking_stop(ke->kh);
                                scanf("%d", &num_packets);
                                _keys_non_blocking_start(ke->kh);
                                break;
                                
                            // Show local statistic
                            case 2:
                                ksnTRUDPstatShow(ke->kc->ku);
                                break;
                                
                            // Send request to show remote peer statistic    
                            case 3:
                                ksnCoreSendCmdto(ke->kc, 
                                        (char*)peer_to, 
                                        CMD_USER + 1, 
                                        CMD_U_STAT, sizeof(CMD_U_STAT));
                                break;
                                
                            case 4:
                                break;
                                
                            default:
                                break;
                        } 
                        
                        if(command >= 0 && command <= 4) {                            
                            if(command != 3 && command != 4)
                                printf("%s\n", PRESS_U);
                            break;                        
                        }
                    }
                    
                    // Exit
                    if(command == 0) {

                        ev_break (ke->ev_loop, EVBREAK_ONE);
                        break;
                    }
                    // Send packages
                    else if(command == 1) {

                        printf("Send %d messages to %s\n",num_packets, peer_to);

                        for(i = 0; i < num_packets; i++) {

                            sprintf(buffer, "#%d: Teoack Hello!", idx++);
                            printf("Send message \"%s\"\n", buffer);
                            ksnCoreSendCmdto(ke->kc, (char*)peer_to, 
                                CMD_USER, buffer, strlen(buffer)+1);
                        }
                        //ksnetEvMgrSetCustomTimer(ke, 3.00); // Set custom timer interval
                    }
                }
            }
        }
        break;            
        
        // Send when DATA received
        case EV_K_RECEIVED:
        {
            // DATA event
            ksnCorePacketData *rd = data;
            
            switch(rd->cmd) {
                
                // Server got DATA from client
                case CMD_USER:
                    printf("Got DATA: %s\n", 
                        (char*)rd->data); 
                    break;
                
                // Server got CONTROL from client
                case CMD_USER + 1: 
                {
                    if(!strcmp((char*)rd->data, CMD_U_STAT)) {
                        char *stat = ksnTRUDPstatShowStr(ke->kc->ku);
                        ksnCoreSendCmdto(ke->kc, rd->from, 
                                CMD_USER + 2, stat, strlen(stat)+1);
                        free(stat);
                    }
                }    
                break;
                    
                // Client got statistic from remote peer (server)    
                case CMD_USER + 2:                    
                    printf("%s%s\n", (char*)rd->data, PRESS_U); 
                    break;
            }
        }
        break;
            
        // Send when ACK received
        case EV_K_RECEIVED_ACK: 
        {
            // ACK event
            ksnCorePacketData *rd = data;
            if(strcmp(peer_to, SERVER_NAME) && strcmp((char*)rd->data, CMD_U_STAT)) {
                printf("Got ACK event to ID %d, data: %s\n", 
                       *(uint32_t*)user_data, (char*)rd->data);
            }
            //ksnetEvMgrSetCustomTimer(ke, 1.00); // Set custom timer interval
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
 * 
 * @return
 */
int main(int argc, char** argv) {

    printf("Teoack multi example ver " TACKM_VERSION ", based on teonet ver. "
            VERSION "\n");
    
    
    // Application parameters
    char *app_argv[] = { "", "peer_to"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Set custom timer interval
    ksnetEvMgrSetCustomTimer(ke, 1.00);
    
    // Start teonet
    ksnetEvMgrRun(ke);
        
    return (EXIT_SUCCESS);
}
