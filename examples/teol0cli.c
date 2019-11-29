/** 
 * \file   teol0cli.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teol0cli.c
 * 
 * Teonet L0 Client example
 *
 * Created on October 8, 2015, 10:13 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ev.h>

#include "ev_mgr.h"
#include "teonet_l0_client.h"

#define TL0C_VERSION "0.0.1"  

int fd;
ev_io w;

/**
 * TCP client callback
 * 
 * Get packet from L0 Server 
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * 
 */
void tcp_read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
        
    // \todo Create L0 client function to combine and split receive buffer
    
    char buf[KSN_BUFFER_DB_SIZE];
    size_t rc = read(w->fd, buf, KSN_BUFFER_DB_SIZE);
    ksnetEvMgrClass *ke = w->data;
    
    // Process received data
    if(rc > 0) {
        
        teoLNullCPacket *sp = (teoLNullCPacket*)buf;
        switch(sp->cmd) {
            
            // Show CMD_ECHO_ANSWER data
            case CMD_ECHO_ANSWER:
            {    
                char *data = sp->peer_name + sp->peer_name_length;
                ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "Receive %d bytes CMD_ECHO_ANSWER: %d bytes data from L0 server, "
                    "from peer %s, data: %s\n", 
                    (int)rc, sp->data_length, sp->peer_name, data);
            } break;
            
            // Show CMD_PEERS_ANSWER data
            case CMD_PEERS_ANSWER:
            {    
                ksnet_arp_data_ar *data_ar = (void*) (sp->peer_name + sp->peer_name_length);
                ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "Receive %d bytes CMD_PEERS_ANSWER: %d bytes data from L0 server, "
                    "from peer %s, data rows: %d\n", 
                    (int)rc, sp->data_length, sp->peer_name, data_ar->length);
                char *header = ksnetArpShowHeader(1);
                char *footer = ksnetArpShowHeader(0);
                printf("%s", header);
                int i = 0;
                for(i = 0; i < data_ar->length; i++) {
                    char *str = ksnetArpShowLine(i+1, data_ar->arp_data[i].name, 
                            &data_ar->arp_data[i].data);
                    printf("%s", str);
                    free(str);
                }
                printf("%s", footer);
                free(header);
                free(footer);
            } break;
            
            default:
                ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "Receive %d bytes UNKNOWN_COMMAND %d: %d bytes data from L0 server, "
                    "from peer %s\n", 
                    (int)rc, sp->cmd, sp->data_length, sp->peer_name);
                break;
        }
        
        printf("Press Ctrl+C to exit\n");
    }
}
        
/**
 * Teonet Events callback
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param event Teonet Event (ksnetEvMgrEvents)
 * @param data Events data
 * @param data_len Data length
 * @param user_data Some user data (may be set in ksnetEvMgrInitPort())
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {

    // Check teonet event
    switch(event) {
        
        // Calls immediately after event manager starts
        case EV_K_STARTED:
        {
            // Connect to L0 Server
            fd = ksnTcpClientCreate(ke->kt, atoi(ke->ksn_cfg.app_argv[2]), 
                    ke->ksn_cfg.app_argv[1]);

            if(fd > 0) {
                
                printf("Start teonet L0 Client example\n");
                
                // Set TCP_NODELAY option
                teosockSetTcpNodelay(fd);
                
                // Create and start TCP watcher (start TCP client processing)
                ev_init (&w, tcp_read_cb);
                ev_io_set (&w, fd, EV_READ);
                w.data = ke;
                ev_io_start (ke->ev_loop, &w);                

                char packet[KSN_BUFFER_SIZE];
                teoLNullCPacket *pkg = (teoLNullCPacket*) packet;

                // Initialize L0 connection
                size_t snd;
                char *host_name = ksnetEvMgrGetHostName(ke);
                size_t pkg_length = teoLNullPacketCreateLogin(packet, KSN_BUFFER_SIZE, 
                        host_name);
                if((snd = write(fd, pkg, pkg_length)) >= 0);                
                ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "Send %d bytes initialize packet to L0 server\n", (int)snd);
                
                // Send get peers request to peer
                char *peer_name = ke->ksn_cfg.app_argv[3]; // Peer name 
                pkg_length = teoLNullPacketCreate(packet, KSN_BUFFER_SIZE, 
                        CMD_PEERS, peer_name, NULL, 0);
                //if((snd = write(fd, pkg, pkg_length)) >= 0);
                teosockSend(fd, (const char *)pkg, pkg_length);
                // if((snd = teoLNullPacketSend(fd, pkg, pkg_length)) >= 0);                
                ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "Send %d bytes packet to L0 server to peer %s, cmd = %d\n", 
                    (int)snd, peer_name, CMD_PEERS);
                
                // Send echo request to peer
                peer_name = ke->ksn_cfg.app_argv[3];    // Peer name 
                char *msg = ke->ksn_cfg.app_argv[4];    // Message
                pkg_length = teoLNullPacketCreate(packet, KSN_BUFFER_SIZE, 
                        CMD_ECHO, peer_name, msg, strlen(msg) + 1);
                //if((snd = write(fd, pkg, pkg_length)) >= 0);
                teosockSend(fd, (const char *)pkg, pkg_length);
                // if((snd = teoLNullPacketSend(fd, pkg, pkg_length)) >= 0);

                ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "Send %d bytes packet to L0 server to peer %s, cmd = %d, data: %s\n", 
                    (int)snd, peer_name, CMD_ECHO, msg);
            }
            
        } break;
        
        default:
            break;
    }
}    

/**
 * Main L0 Client example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 * 
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teol0cli example ver " TL0C_VERSION ", "
           "based on teonet ver. " VERSION "\n");
    
    // Application parameters
    const char *app_argv[] = { "", "l0_server", "port", "peer_to", "message"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 5;
    app_param.app_argv = app_argv;
    app_param.app_descr = NULL;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    return (EXIT_SUCCESS);
}

