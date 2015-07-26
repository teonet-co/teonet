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
    int close_flg = 0;
    
    // Read data
    int read_len = read(w->fd, buffer, BUF_LEN-1);
    buffer[read_len] = 0;
    
    //Show message
    printf("Server received %d bytes data: %s\n", read_len, buffer); 
    
    // Process received data
    if(read_len) {
        
        // Create temporary buffer
        char *d = strdup(buffer); // Copy buffer
        d[strcspn(d, "\r\n")] = 0; // Remove trailing CRLF

        // Check quit client command
        if(!strcmp(d,"quit")) {            
            printf("The connection to client %d was closed\n", w->fd);
            close_flg = 1;
        }
        
        // Check destroy server command
        else if(!strcmp(d,"destroy")) {            
            printf("The TCP server %d was destroyed\n", w->fd);
            ksnTcpServerStopAll(((ksnetEvMgrClass *)w->data)->kt);
        }

        // Check destroy server command
        else if(!strcmp(d,"close_all")) {            
            int fd = ksnTcpGetServer(((ksnetEvMgrClass *)w->data)->kt, w->fd);
            if(fd) {
                printf("Close all TCP server %d clients\n", fd);            
                ksnTcpServerStopAllClients(((ksnetEvMgrClass *)w->data)->kt, fd);
                printf("OK\n");
            }
            else {
                printf("Can't find server of %d client\n", w->fd);
            }
        }

        // Send the data back to client
        else {
            write(w->fd, buffer, read_len);            
        }
        
        // Free temporary buffer
        free(d);
    }
    
    // Client has disconnected - stop this watcher
    else {        
        printf("The client %d has disconnected\n", w->fd);
        close_flg = 1;
    }
    
    // Close connection and free watcher
    if(close_flg) {        
        ksnTcpCbStop(loop, w, 1);
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
void tcp_server_accept_cb(struct ev_loop *loop, ev_ksnet_io *w,
                       int revents, int fd) {
    
    printf("Client %d connected\n", fd);
    
    // Connect clients fd to event manager
    ksnTcpCb(((ksnetEvMgrClass *)w->io.data)->ev_loop, w, fd, 
            tcp_server_receive_cb, w->io.data);
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
