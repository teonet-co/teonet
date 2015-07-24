/**
 * File:   net_tcp.c
 * Author: Kirill Scherba
 *
 * Created on March 29, 2015, 6:43 PM
 * Updated to use in libksmesh on May 06, 2015, 21:55
 * Adapted to use in libteonet on July 24, 2015, 11:56
 *
 * TCP client/server based on teonet event manager
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "ev_mgr.h"
#include "net_tcp.h"
#include "utils/rlutil.h"

#define KSNET_TRY_PORT 1000
#define KSNET_IPV6_ENABLE

// Local functions
int ksnTcpServerStart(ksnTcpClass *kt, int *port);
void ksnTcpServerAccept(struct ev_loop *loop, ev_io *w, int revents);


// Subroutines
#define kev ((ksnetEvMgrClass*)kt->ke)
#define ev_ksnet_io_start(loop, ev, cb, fd, events) \
    ev_io_init (ev, cb, fd, events); \
    ev_io_start (loop, ev)


/******************************************************************************/
/* TCP Client/Server module methods                                         */
/*                                                                            */
/******************************************************************************/

/**
 * Initialize the TCP client/server module
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @return
 */
ksnTcpClass *ksnTcpInit(void *ke) {

    ksnTcpClass *kt = malloc(sizeof(ksnTcpClass));
    kt->ke = ke;

    return kt;
}

/**
 * Destroy the TCP client/server module
 *
 * @param kt
 */
void ksnTcpDestroy(ksnTcpClass *kt) {

    ksnetEvMgrClass *ke = kt->ke;
    
    free(kt);
    ke->kt = NULL;
}

/******************************************************************************/
/* TCP Server methods                                                                 */
/*                                                                            */
/******************************************************************************/

/**
 * Create TCP server and add it to Event manager
 *
 * @param port
 * @param ksnet_accept_cb
 * @param data
 * @return
 */
int ksnTcpServerCreate(
    ksnTcpClass *kt,
    int port,
    void (*ksnet_accept_cb) (struct ev_loop *loop, struct ev_ksnet_io *watcher, int revents, int fd),
    void *data,
    int *port_created) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&((ksnetEvMgrClass*)kt->ke)->ksn_cfg, DEBUG,
            "%sTCP Server:%s Create TCP server at port %d\n", 
            ANSI_MAGENTA, ANSI_NONE, port);
    #endif

    ev_ksnet_io *w_accept = (ev_ksnet_io*) malloc(sizeof(struct ev_ksnet_io));

    // Start TCP server
    int sd, server_port = port;
    if( (sd = ksnTcpServerStart(kt, &server_port)) > 0 ) {

        *port_created = server_port;

        // Initialize and start a watcher to accepts client requests
        w_accept->tcpServerAccept_cb = ksnTcpServerAccept; // TCP Server Accept callback
        w_accept->ksnet_cb = ksnet_accept_cb; // User Server Accept callback
        w_accept->events = EV_READ; // Events
        w_accept->fd = sd; // TCP Server socket
        w_accept->data = data; // Some user data
        w_accept->io.data = kev;

        // Start watcher
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wstrict-aliasing"
        ev_ksnet_io_start (kev->ev_loop, &w_accept->io,
                           w_accept->tcpServerAccept_cb, w_accept->fd, EV_READ);
        #pragma GCC diagnostic pop
    }
    else *port_created = 0;

    return sd;
}

/**
 * Create server socket, bund to port and start listen in
 *
 * @param port [in/out] Servers port
 * @return
 */
int ksnTcpServerStart(ksnTcpClass *kt, int *port) {

    int sd = 0;

    #ifdef KSNET_IPV6_ENABLE
    struct sockaddr_in6 addr;
    #else
    struct sockaddr_in addr;
    #endif

    int try_port = KSNET_TRY_PORT;
    while(try_port--) {

        /***********************************************************************/
        /* The socket() function returns a socket descriptor, which represents */
        /* an endpoint.  Get a socket for address family AF_INET6 to           */
        /* prepare to accept incoming connections on IPv6 and IPv4.            */
        /***********************************************************************/
        #ifdef KSNET_IPV6_ENABLE
        if ((sd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
            ksnet_printf(&kev->ksn_cfg, ERROR_M, 
                    "%sTCP Server:%s socket error\n", 
                    ANSI_MAGENTA, ANSI_NONE);
            return -1;
        }
        #else
        // Create IPv4 only server socket
        if( (sd = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        {
            ksnet_printf(ERROR, "%sTCP Server:%s: socket error", 
                    ANSI_MAGENTA, ANSI_NONE);
            return -1;
        }
        #endif

        // Set server socket options
        int yes = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            // error handling
            ksnet_printf(&kev->ksn_cfg, ERROR_M,
                    "%sTCP Server:%s can't set socket options\n", 
                    ANSI_MAGENTA, ANSI_NONE);
            return -1;
        }

        #ifdef KSNET_IPV6_ENABLE
        bzero(&addr, sizeof(addr));
        addr.sin6_flowinfo = 0;
        addr.sin6_family = AF_INET6;
        addr.sin6_port   = htons(*port);
        addr.sin6_addr = in6addr_any;
        #else
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        #endif

        // Bind socket to address
        if (bind(sd, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
            ksnet_printf(&kev->ksn_cfg, ERROR_M,
                    "%sTCP Server:%s bind on port %d error\n", 
                    ANSI_MAGENTA, ANSI_NONE, *port);
            close(sd);
            if(try_port) (*port)++;
        }
        else break;
    }

    // Start listing on the socket
    if (listen(sd, 2) < 0) {
        ksnet_printf(&kev->ksn_cfg, ERROR_M,
                "%sTCP Server:%s listen on port %d error\n", 
                ANSI_MAGENTA, ANSI_NONE, *port);
        return -1;
    }

    // Set non block mode
    set_nonblock(sd);

    // Server welcome message
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG,
            "%sTCP Server:%s started on port %d, socket fd %d ...\n", 
            ANSI_MAGENTA, ANSI_NONE, *port, sd);
    #endif

    return sd;
}

/**
 * Accept TCP server client requests callback
 *
 * @param loop
 * @param watcher
 * @param revents
 */
void ksnTcpServerAccept(struct ev_loop *loop, ev_io *w, int revents) {

    ev_ksnet_io *watcher = (ev_ksnet_io *) w;
    ksnetEvMgrClass *ke = (ksnetEvMgrClass *) watcher->io.data;

    // Check event
    if(EV_ERROR & revents) {

        // Close server signal got
        if(revents == EV_ERROR + EV_READ + EV_WRITE) {

            #ifdef DEBUG_KSNET
            ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "%sTCP Server:%s accept, server with FD %d was closed\n", 
                    ANSI_MAGENTA, ANSI_NONE, w->fd);
            #endif

            // Free watcher
            //close(w->fd);
            free(w);
        }
        else {

            #ifdef DEBUG_KSNET
            ksnet_printf(&ke->ksn_cfg, DEBUG,
                    "%sTCP Server:%s accept, got invalid event: 0x%04X\n", 
                    ANSI_MAGENTA, ANSI_NONE, revents);
            #endif
        }


        return;
    }

    // Define sockaddr structure
    #ifdef KSNET_IPV6_ENABLE
    struct sockaddr_in6 client_addr;
    #else
    struct sockaddr client_addr;
    #endif
    socklen_t client_len = sizeof(client_addr);

    // Accept client request
    int client_sd = accept(watcher->io.fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_sd < 0) {
        ksnet_printf(&ke->ksn_cfg, ERROR_M, 
                "%sTCP Server:%s accept, accept error\n",
                ANSI_MAGENTA, ANSI_NONE);
        return;
    }

    // Increment total_clients count
//    total_clients ++;

    // Get Client IPv4 & port number
    int client_family = AF_INET;
    char client_ip[100];
    struct sockaddr_in *addr = (struct sockaddr_in*) &client_addr;
    #ifndef KSNET_IPV6_ENABLE
    strcpy(client_ip, inet_ntoa(addr->sin_addr))
    printf("IP address is: %s\n", client_ip);
    #endif
    int client_port = (int) ntohs(addr->sin_port);
    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG,
            "%sTCP Server:%s Client port is: %d\n", 
            ANSI_MAGENTA, ANSI_NONE, client_port);
    #endif

    // Sockets Layer Call: inet_ntop()
    #ifdef KSNET_IPV6_ENABLE
    inet_ntop(AF_INET6, &(client_addr.sin6_addr), client_ip, sizeof(client_ip));
    // Check client inet family
    #define IPV4_PREF "::ffff:"
    if(strstr(client_ip, IPV4_PREF) == client_ip) {
        memmove(client_ip, client_ip + sizeof(IPV4_PREF) - 1,
                strlen(client_ip) - (sizeof(IPV4_PREF) - 1) + 1 );
        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG,
                "%sTCP Server:%s Client IP address is: %s\n", 
                ANSI_MAGENTA, ANSI_NONE, client_ip);
        #endif
    } else {
        client_family = AF_INET6;
        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG,
                "%sTCP Server:%s Client IPv6 address is: %s\n", 
                ANSI_MAGENTA, ANSI_NONE, client_ip);
        #endif
    }
    #endif

    #ifdef DEBUG_KSNET
    ksnet_printf(
        &ke->ksn_cfg,
        DEBUG,
        "%sTCP Server:%s Successfully connected with client (%d), from IP%s: %s.\n",
        ANSI_MAGENTA, ANSI_NONE, 
        client_sd, client_family == AF_INET6 ? "v6" : "" , client_ip
    );
//    ksnet_printf(&ke->ksn_cfg, DEBUG, "TCP Server: %d client(s) connected.\n", total_clients);
    #endif

    // Execute ksnet callback (to Initialize and start watcher to read Servers client requests)
    watcher->ksnet_cb(loop, watcher, revents, client_sd);

    // Add socket FD to the Event manager FD list
    //ksnet_EventMgrAddFd(client_sd);
}

/**
 * Set callback to accepted client
 *
 * @param loop
 * @param fd
 * @param ksnet_read_cb
 */
void ksnTcpCb(
    struct ev_loop *loop,
    int fd,
    void (*ksnet_cb)(struct ev_loop *loop, ev_io *watcher, int revents),
              void* data) {

    // Define READ event (event when TCP server data is available for reading)
    struct ev_io *w_client = (struct ev_io*) malloc (sizeof(struct ev_io));
    w_client->data = data;
    ev_io_init (w_client, ksnet_cb, fd, EV_READ);
    ev_io_start (loop, w_client);
}


/******************************************************************************/
/* TCP Client methods                                                                  */
/*                                                                            */
/******************************************************************************/

int ksnTcpClientCreate(ksnTcpClass *kt, int port, const char *server) {

    /* Variable and structure definitions. */
    int sd, rc;
    struct sockaddr_in serveraddr;
    struct hostent *hostp;

    /* The socket() function returns a socket */
    /* descriptor representing an endpoint. */
    /* The statement also identifies that the */
    /* INET (Internet Protocol) address family */
    /* with the TCP transport (SOCK_STREAM) */
    /* will be used for this socket. */
    /******************************************/
    /* get a socket descriptor */
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        ksnet_printf(&kev->ksn_cfg, ERROR_M, 
                "%sTCP client:%s Client-socket() error\n", 
                ANSI_LIGHTMAGENTA, ANSI_NONE);
        return(-1);
    }
    else {
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG, 
                "%sTCP client:%s Client-socket() OK\n", 
                ANSI_LIGHTMAGENTA, ANSI_NONE);
        #endif
    }

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG,
            "%sTCP client:%s Connecting to the f***ing %s, port %d ...\n", 
            ANSI_LIGHTMAGENTA, ANSI_NONE, server, port);
    #endif

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    if((serveraddr.sin_addr.s_addr = inet_addr(server)) == (unsigned long) INADDR_NONE)
    {

        /* When passing the host name of the server as a */
        /* parameter to this program, use the gethostbyname() */
        /* function to retrieve the address of the host server. */
        /***************************************************/
        /* get host address */
        hostp = gethostbyname(server);
        if(hostp == (struct hostent *)NULL)
        {
            ksnet_printf(&kev->ksn_cfg, ERROR_M,
                    "%sTCP client:%s HOST NOT FOUND --> h_errno = %d\n", 
                    ANSI_LIGHTMAGENTA, ANSI_NONE, h_errno);
            close(sd);
            return(-1);
        }
        memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
    }

    /* After the socket descriptor is received, the */
    /* connect() function is used to establish a */
    /* connection to the server. */
    /***********************************************/
    /* connect() to server. */
    if((rc = connect(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
    {
        ksnet_printf(&kev->ksn_cfg, ERROR_M, 
                "%sTCP client:%s Client-connect() error\n", 
                ANSI_LIGHTMAGENTA, ANSI_NONE);
        close(sd);
        return(-1);
    }
    else
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG, 
            "%sTCP client:%s Connection established...\n", 
            ANSI_LIGHTMAGENTA, ANSI_NONE);
    #endif

    // Set non block mode
    set_nonblock(sd);

    return sd;
}
