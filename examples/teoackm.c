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
#define CMD_U_RESET "reset"
#define CMD_U_DATA_OR_STAT "data_or_stat"

#define SERVER_NAME "none"

const char* PRESS_A = "(Press A to return to main application menu)";
int show_data_or_statistic_at_server = 1;

/**
 * Application states
 */
enum {
    
    STATE_NONE,
    STATE_WAIT_KEY,
    STATE_WAIT_STRING
            
} state;

int app_state = STATE_NONE; ///< Application state

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
                printf("Peer %s connected at %s:%d \n", 
                       rd->from, rd->addr, rd->port);
        }    
        break;
            
        // Send when peer disconnected
        case EV_K_DISCONNECTED: 
        {
            ksnCorePacketData *rd = data;
            if(rd->from != NULL) {
                printf("Peer %s was disconnected \n", rd->from);
            }
            else printf("Peer was disconnected\n");
        }    
        break;
        
        // Check hotkey
        case EV_K_HOTKEY:
        {
            int i, idx, num_packets, command = *(int*)data;
            
            // Check hotkey
            if(app_state == STATE_WAIT_KEY) {
                
                app_state = STATE_NONE;
                switch(command) {

                    // Exit
                    case '0':
                        ev_break (ke->ev_loop, EVBREAK_ONE);
                        break;

                    // Send packets
                    case '1':
                        printf("How much package to send (0 - to exit): ");
                        _keys_non_blocking_stop(ke->kh);
                        scanf("%d", &num_packets);
                        _keys_non_blocking_start(ke->kh);

                        printf("Send %d messages to %s\n",num_packets, peer_to);
                        idx = 0;
                        for(i = 0; i < num_packets; i++) {
                            char buffer[KSN_BUFFER_DB_SIZE];
                            sprintf(buffer, "#%d: Teoack Hello!", ++idx);
                            printf("Send message \"%s\"\n", buffer);
                            ksnCoreSendCmdto(ke->kc, (char*)peer_to, 
                                CMD_USER, buffer, strlen(buffer)+1);
                        }   
                        printf("\nACK ID: ");
                        break;

                    // Show local statistic
                    case '2':
                        ksnTRUDPstatShow(ke->kc->ku);
                        printf("%s\n", PRESS_A); 
                        break;

                    // Send request to show remote peer statistic    
                    case '3':
                        ksnCoreSendCmdto(ke->kc, 
                                (char*)peer_to, 
                                CMD_USER + 1, 
                                CMD_U_STAT, sizeof(CMD_U_STAT));
                        break;

                    // Send reset
                    case '4':
                    {
                        ksnet_arp_data *arp = ksnetArpGet(ke->kc->ka, peer_to);
                        if(arp != NULL) {
                            
                            // Make address from string
                            struct sockaddr_in remaddr; // remote address
                            const socklen_t addrlen = sizeof(remaddr); // length of addresses
//                            memset((char *) &remaddr, 0, addrlen);
//                            remaddr.sin_family = AF_INET;
//                            remaddr.sin_port = htons(arp->port);
//                            #ifndef HAVE_MINGW
//                            if(inet_aton(arp->addr, &remaddr.sin_addr) == 0) {
//                                //return(-2);
//                            }
//                            #else
//                            remaddr.sin_addr.s_addr = inet_addr(addr);
//                            #endif
                            if(!ksnTRUDPmakeAddr(arp->addr, arp->port, 
                                    (__SOCKADDR_ARG) &remaddr, &addrlen)) {
                                
                                ksnTRUDPresetSend(ke->kc->ku, ke->kc->fd, 
                                        (__CONST_SOCKADDR_ARG) &remaddr);
                                ksnTRUDPreset(ke->kc->ku, 
                                        (__CONST_SOCKADDR_ARG) &remaddr, 0);
                                ksnTRUDPstatReset(ke->kc->ku);
                            }
                        }
                        
                        // Show menu
                        ke->event_cb(ke, EV_K_USER , NULL, 0, NULL);
                    }
                    break;
                    
                    // Show data or statistic at server
                    case '5':
                    {
                        show_data_or_statistic_at_server = !show_data_or_statistic_at_server;
                        ksnet_arp_data *arp = ksnetArpGet(ke->kc->ka, peer_to);
                        if(arp != NULL) {
                            // Make address from string
                            struct sockaddr_in remaddr; // remote address
                            const socklen_t addrlen = sizeof(remaddr); // length of addresses
                            if(!ksnTRUDPmakeAddr(arp->addr, arp->port, 
                                    (__SOCKADDR_ARG) &remaddr, &addrlen)) {
                                
                                //Make command string
                                char *command = ksnet_formatMessage("%s %d", 
                                        CMD_U_DATA_OR_STAT, 
                                        show_data_or_statistic_at_server 
                                );
                                
                                // Send command to server
                                ksnCoreSendCmdto(ke->kc, 
                                        (char*)peer_to, 
                                        CMD_USER + 1, 
                                        command, strlen(command) + 1
                                );
                                
                                // Free command string memory
                                free(command);
                            }
                        }
                        
                        // Show menu
                        ke->event_cb(ke, EV_K_USER , NULL, 0, NULL);
                    }
                    break;

                    default:
                        break;
                }
            }         
        }
        break;
            
        // Send by timer
        case EV_K_USER:    
        case EV_K_TIMER:
        {
            if(strcmp(peer_to, ksnetEvMgrGetHostName(ke))) {

                // If peer_to is connected
                if (ksnetArpGet(ke->kc->ka, peer_to) != NULL) {

                    ksnetEvMgrSetCustomTimer(ke, 0.00); // Stop timer
                    
                    printf("\n"
                           "Multi send test menu:\n"
                           "\n"
                           "  1 - send packets\n"
                           "  2 - show TR-UDP statistics\n" 
                           "  3 - get and show remote peer TR-UDP statistics\n" 
                           "  4 - send TR-UDP reset\n" 
                           "  5 - show data or TR-UDP statistic screen at server (now: %s)\n"
                           "  0 - exit\n"
                           "\n"
                           "teoackm $ "
                           , show_data_or_statistic_at_server ? "data" : "statistic" 
                    );
                    fflush(stdout);
                    app_state = STATE_WAIT_KEY;
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
                    if(strcmp(peer_to, SERVER_NAME) || show_data_or_statistic_at_server) {
                        printf("Got DATA: %s\n", (char*)rd->data); 
                    }
                    break;
                
                // Server got CONTROL from client
                case CMD_USER + 1: 
                {
                    // Send statistic back
                    if(!strcmp((char*)rd->data, CMD_U_STAT)) {
                        
                        char *stat = ksnTRUDPstatShowStr(ke->kc->ku);
                        ksnCoreSendCmdto(ke->kc, rd->from, 
                                CMD_USER + 2, stat, strlen(stat)+1);
                        free(stat);
                    }
                    
                    // Show data or statistic
                    else if(!strncmp((char*)rd->data, CMD_U_DATA_OR_STAT, 
                            strlen(CMD_U_DATA_OR_STAT))) {
                        
                        sscanf((char*)rd->data, "%*s %d", 
                                &show_data_or_statistic_at_server);
                        
                        printf("show_data_or_statistic_at_server %d\n", 
                                show_data_or_statistic_at_server);
                        
                        // Show DATA - stop TR-UDP statistic
                        if(show_data_or_statistic_at_server) {
                            
                            ke->ksn_cfg.show_tr_udp_f = 0;
                        }
                        //Show Statistic - start TR-UDP statistic
                        else {
                            ke->ksn_cfg.show_tr_udp_f = 1;
                        }
                    }
                }    
                break;
                    
                // Client got statistic from remote peer (server)    
                case CMD_USER + 2:                    
                    printf("%s%s\n", (char*)rd->data, PRESS_A); 
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
                printf("%d ", *(uint32_t*)user_data);
                fflush(stdout);
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
