/** 
 * File:   teotcp.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet TCP Client / Server example. 
 * Simple TCP echo server and client connected to it.
 *
 * Created on July 24, 2015, 2:23 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ev_mgr.h"

#define TTCP_VERSION VERSION    
    
/**
 * TCP server receive data from client callback
 * 
 * @param loop
 * @param w
 * @param revents
 */
void tcp_server_receive_cb(struct ev_loop *loop, ev_io *w, int revents) {
    
    #define BUF_LEN 1024
    char buffer[BUF_LEN];
    
    // Read data
    int read_len = read(w->fd, buffer, BUF_LEN-1);
    buffer[read_len] = 0;
    
    //Show message
    printf("Server receive %d bytes data: %s\n", read_len, buffer); 
    
    // Process received data
    if(read_len) {
        
        // Send the data back to client
        write(w->fd, buffer, read_len);

        // Check quit command
        buffer[strcspn(buffer, "\r\n")] = 0; // Remove trailing CRLF
        if(!strcmp(buffer,"quit")) {
            printf("The connection to client %d is closed\n", w->fd);
            ksnTcpCbStop(loop, w);
        }
    }
    
    // Client was disconnected - stop this watcher
    else {
        
        printf("The client %d was disconnected\n", w->fd);
        ksnTcpCbStop(loop, w);
    }
}

/**
 * TCP server accept callback
 *
 * @param loop
 * @param watcher
 * @param revents
 * @param fd
 */
void tcp_server_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *w,
                       int revents, int fd) {
    
    printf("Client %d connected\n", fd);
    
    ksnTcpCb(((ksnetEvMgrClass *)w->io.data)->ev_loop, fd, 
            tcp_server_receive_cb,NULL);
}

/**
 * Teonet Events callback
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {
    
    switch(event) {
        
        // Calls immediately after event manager starts
        case EV_K_STARTED:
        {
            // Type of application (client or server)
            printf("Type of application: %s\n",  ke->ksn_cfg.app_argv[1]); 
            
            // Server
            if(!strcmp(ke->ksn_cfg.app_argv[1], "server")) {
                
                // Start TCP server
                int port_created, 
                    fd = ksnTcpServerCreate(ke->kt, ke->ksn_cfg.port, 
                        tcp_server_accept_cb, NULL, &port_created);
            }
            
            //Client
            else if(!strcmp(ke->ksn_cfg.app_argv[1], "client")) {
                
            }
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

    printf("Teotcp ver " TTCP_VERSION "\n");
    
    // Application parameters
    char *app_argv[] = { "", "app_type"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
