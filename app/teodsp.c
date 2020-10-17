/**
 * \file   teodsp.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet C dispatcher
 *
 * Features:
 *
 * The basic commands of the application (API):
 *
 *
 * Created on 2016-03-27 15:40:07
 * 
 */

#define TDB_VERSION "0.0.1"
#define APPNAME _ANSI_LIGHTMAGENTA "teodsp" _ANSI_NONE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "ev_mgr.h"



typedef struct usock_class {
    
    ksnetEvMgrClass *ke;
    ev_io *w;
    
} usock_class;

static int usock_disconnect(usock_class *us);


#define MODULE _ANSI_LIGHTBLUE "usock" _ANSI_NONE

#define HTTP_SUF " HTTP/1.0\r\n\r\n"

static usock_class *usock_init(ksnetEvMgrClass *ke) {
    
    usock_class *us = malloc(sizeof(usock_class));
    us->ke = ke;
    us->w = NULL;
    
    return us;
}

static void usock_destroy(usock_class *us) {
    
    ksn_puts(us->ke, MODULE, DEBUG_VV, "destroyed ...");
    if(us->w != NULL) usock_disconnect(us);
    free(us);
}

static void usock_receive_cb(EV_P_ ev_io *w, int revents) {
    
    const size_t BUFFER_LEN = 1024*10; // 10 Kb buffer
    usock_class *us = w->data;
    char buf[BUFFER_LEN];
    int  rc;
    
    // Read UNIX socket answer
    //rc=recv(w->fd, buf, BUFFER_LEN, 0);
    rc = read(w->fd, buf, BUFFER_LEN);    
    ksn_printf(us->ke, MODULE, DEBUG, "got an answer, %d bytes\n", rc);
    if(rc <= 0) {
        if(rc == 0) {
            usock_disconnect(w->data);
        }
        else {
            perror("read error");
            //return -2;
        }        
    }
    
    // Process answer data - send message to application event callback
    else {
        buf[rc] = 0;
        if(us->ke->event_cb != NULL) 
            us->ke->event_cb(us->ke, EV_U_RECEIVED, buf, rc, us);
    }
}

static int usock_connect(usock_class *us, char *socket_path) {
    
    struct sockaddr_un addr;
    int fd;    
    
    // Creates  an  endpoint  for  communication  and returns a descriptor
    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        return -1;
    }
    
    // Create address structure from socket path
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    
    // Connect to socket
    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        return -2;
    }
    
    // Set non block mode
    teosockSetBlockingMode(fd, TEOSOCK_NON_BLOCKING_MODE);
    
    // Add UNIX socket watcher to the event manager
    if(us->w != NULL) usock_disconnect(us);
    us->w = malloc(sizeof(ev_io));
    ev_io_init(us->w, usock_receive_cb, fd, EV_READ);
    us->w->data = us;
    ev_io_start(us->ke->ev_loop, us->w);
    
    ksn_printf(us->ke, MODULE, MESSAGE, 
            "connected to UNIX socket: %s\n", socket_path);
    
    return fd;
}

static int usock_disconnect(usock_class *us) {
    
    ksn_puts(us->ke, MODULE, MESSAGE, "disconnected ...");      
    
    if(us->w != NULL) {
        ev_io_stop(us->ke->ev_loop, us->w);
        close(us->w->fd);
        free(us->w);
        us->w = NULL;
    }
    
    return 0;
}

static int usock_send_request(usock_class *us, char *request) {
    
    int  rc = strlen(request);
    if(rc > 0) {
        if(write(us->w->fd, request, rc) != rc) {
        //if(send(fd, buf, rc, 0) != rc) {

            if (rc > 0) fprintf(stderr,"partial write need");
            else {
                perror("write error");
                return -1;
            }
        }
        ksn_printf(us->ke, MODULE, DEBUG, "send request, %d bytes: %s\n", rc, 
                trimlf(request));
    }
    else perror("send buffer create error");
    
    return 0;
}

static int usock_send_request_http(usock_class *us, char *request) {

    size_t request_len = strlen(request);
    char request_http[request_len + 1 + sizeof(HTTP_SUF)];
    memcpy(request_http, request, request_len);
    memcpy(request_http + request_len, HTTP_SUF, sizeof(HTTP_SUF));
    request_http[request_len + sizeof(HTTP_SUF)] = 0;
    
    return usock_send_request(us, request_http);
}

/**
 * Teonet event handler
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 * @param user_data
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {

    // Switch Teonet event
    switch(event) {
        
        // After start
        case EV_K_STARTED:
            
            ksn_puts(ke, APPNAME, MESSAGE, "application started");
            
            usock_class *us = usock_init(ke);
            ke->user_data = us;
            
            // Connect to Unix socket and send test request
            char *socket_path = "/var/run/docker.sock"; // Docker Unix socket path        
            int fd = usock_connect(us, socket_path);           
            if(fd > 0) {
                // Test requests
                usock_send_request_http(us, "GET /info");
                usock_send_request_http(us, "GET /containers/json");
                usock_send_request_http(us, "GET /networks");
            }
            break;

        // Before stop
        case EV_K_STOPPED_BEFORE:
            usock_destroy(ke->user_data);
            ksn_puts(ke, APPNAME, MESSAGE, "application stopped");
            break;
            
        // UNIX socket data received
        case EV_U_RECEIVED:
            printf("%s\n", (char*)data);
            break;
            
        // DATA received
        case EV_K_RECEIVED:
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

    printf("Teonet C dispatcher ver " TDB_VERSION ", based on teonet ver "
            "%s" "\n", teoGetLibteonetVersion());

    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb, READ_ALL);

    // Set application type
    teoSetAppType(ke, "teo-dsp");
    teoSetAppVersion(ke, TDB_VERSION);

    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
