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
        
    char buf[KSN_BUFFER_DB_SIZE];
    size_t rc = read(w->fd, buf, KSN_BUFFER_DB_SIZE);
    ksnetEvMgrClass *ke = w->data;
    
    // Process received data
    if(rc > 0) {
        
        teoLNullCPacket *sp = (teoLNullCPacket*)buf;
        char *data = sp->peer_name + sp->peer_name_length;
        
        ksnet_printf(&ke->ksn_cfg, DEBUG,
            "Receive %d bytes: %d bytes data from L0 server, "
            "from peer %s, data: %s\n", 
            (int)rc, sp->data_length, sp->peer_name, data);
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
            fd = ksnTcpClientCreate(ke->kt, 9000, "127.0.0.1");

            if(fd > 0) {
                
                printf("Start teonet L0 Client example\n");
                
                // Set TCP_NODELAY option
                set_tcp_nodelay(fd);
                
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
                size_t pkg_length = teoLNullInit(packet, KSN_BUFFER_SIZE, 
                        host_name);
                if((snd = write(fd, pkg, pkg_length)) >= 0);                
                ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "Send %d bytes initialize packet to L0 server\n", (int)snd);
                
                // Send message to peer
                char *peer_name = "teostream";
                char *msg = "Hello";
                pkg_length = teoLNullPacketCreate(packet, KSN_BUFFER_SIZE, 
                        CMD_ECHO, peer_name, msg, strlen(msg) + 1);
                if((snd = write(fd, pkg, pkg_length)) >= 0);
                ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "Send %d bytes packet to L0 server to peer %s, data: %s\n", 
                    (int)snd, peer_name, msg);
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
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    return (EXIT_SUCCESS);
}

