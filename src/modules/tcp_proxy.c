/** 
 * \file   tcp_proxy.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * # Teonet TCP Proxy module
 * 
 * See example: 
 * 
 * See test: test_tcp_proxy.c
 *
 * *Created on September 8, 2015, 1:59 AM*
 * 
 * ### Description:
 * 
 */

#include <stdlib.h>

#include "tcp_proxy.h"
#include "ev_mgr.h"
#include "net_core.h"
#include "utils/rlutil.h"

// Local function definition
int ksnTCPProxyServerStart(ksnTCPProxyClass *tp);
void ksnTCPProxyServerStop(ksnTCPProxyClass *tp);
void ksnTCPProxyServerClientConnect(ksnTCPProxyClass *tp, int fd);
void ksnTCPProxyServerClientDisconnect(ksnTCPProxyClass *tp, int fd);

#define kev ((ksnetEvMgrClass*)tp->ke)

// Initialize / Destroy functions ---------------------------------------------

/**
 * Initialize TCP Proxy module 
 * 
 * @param ke Pointer to the 
 * @return Pointer to ksnTCPProxyClass
 */
ksnTCPProxyClass *ksnTCPProxyInit(void *ke) {
    
    ksnTCPProxyClass *tp = malloc(sizeof(ksnTCPProxyClass));
    tp->map = pblMapNewHashMap();
    tp->ke = ke;
    tp->fd = 0;  
    
    ksnTCPProxyServerStart(tp);
    
    return tp;
}

/**
 * Destroy TCP Proxy module
 * 
 * @param tp
 */
void ksnTCPProxyDestroy(ksnTCPProxyClass *tp) {
    
    if(tp != NULL) {

        ksnTCPProxyServerStop(tp); // Stop TCP Proxy server
        pblMapFree(tp->map); // Free clients map
        free(tp); // Free class memory
    }
}

// Common functions -----------------------------------------------------------

//! \file   tcp_proxy.c
//! * Client functions: 

//! \file   tcp_proxy.c
//!     * Connect to TCP Proxy server: ksnTCPProxyConnetc()

/**
 * Connect to TCP Proxy server
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * @return 0 - Successfully connected
 */
int ksnTCPProxyConnetc(ksnTCPProxyClass *tp) {
   
    // Get address and port from teonet config
    
    // Connect to R-Host TCP Server
    
    // Register connection (in ARP ?)
    
    // \todo
    
    return 0;
}

//! \file   tcp_proxy.c
//! * Server functions: 

/**
 * TCP Proxy server client callback
 * 
 * Get packet from TCP Proxy client connection and resend it to UDP Proxy
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 */
void cmd_tcpp_read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {

    size_t data_len = KSN_BUFFER_SIZE; // Buffer length
    ksnTCPProxyClass *tp = w->data; // Pointer to ksnTCPProxyClass
    char data[data_len]; // Buffer
    
    // Read TCP data
    ssize_t received = read(w->fd, data, data_len);
    ksnet_printf(&kev->ksn_cfg, DEBUG, // \todo Change type to DEBUG_VV after debugging
            "%sTCP Proxy:%s "
            "Got something from fd %d w->events = %d, received = %d ...\n", 
            ANSI_YELLOW, ANSI_NONE, w->fd, w->events, (int)received);
    
    // Disconnect client:
    // Close UDP and TCP connections and Remove data from TCP Proxy Clients map
    if(!received) {
        
        ksnTCPProxyServerClientDisconnect(tp, w->fd);
    }
}

/**
 * TCP Proxy server accept callback
 * 
 * Register client, create UDP Proxy client/server, create event watchers with 
 * clients callback
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * @param fd File description of created client connection
 */
inline void cmd_tcpp_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *w,
                       int revents, int fd) {
    
    ksnTCPProxyServerClientConnect(w->data, fd);    
}

//! \file   tcp_proxy.c
//!     * Start TCP Proxy server: ksnTCPProxyServerStart()
/**
 * Start TCP Proxy server
 * 
 * Create and start TCP Proxy server
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * @return If return value > 0 than server was created successfully
 */
int ksnTCPProxyServerStart(ksnTCPProxyClass *tp) {
    
    // Create TCP server at port, which will wait client connections
    int fd = 0, port_created;
    if(kev->ksn_cfg.tcp_allow_f && (fd = ksnTcpServerCreate(
                kev->kt, 
                kev->ksn_cfg.tcp_port,
                cmd_tcpp_accept_cb, 
                tp, 
                &port_created)) > 0) {
        
        ksnet_printf(&kev->ksn_cfg, MESSAGE, 
                "%sTCP Proxy:%s TCP Proxy server fd %d started at port %d\n", 
                ANSI_YELLOW, ANSI_NONE,
                fd, port_created);
        
        kev->ksn_cfg.tcp_port = port_created;
        tp->fd = fd;
    }
    
    return fd;
}

//! \file   tcp_proxy.c
//!     * Stop TCP Proxy server: ksnTCPProxyServerStop()
/**
 * Stop TCP Proxy server
 * 
 * Disconnect all connected clients and and stop TCP Proxy server
 * 
 * @param tp Pointer to ksnTCPProxyClass
 */
void ksnTCPProxyServerStop(ksnTCPProxyClass *tp) {
    
    // If server started
    if(kev->ksn_cfg.tcp_allow_f && tp->fd) {
        
        // \todo Test it - did nit show messages in ksnTCPProxyServerClientDisconnect
        printf("ksnTCPProxyServerStop\n");
        // Disconnect all clients
        PblIterator *it = pblMapIteratorNew(tp->map);
        if(it != NULL) {

            while(pblIteratorHasPrevious(it)) {
                void *entry = pblIteratorPrevious(it);
                int *fd = (int *) pblMapEntryKey(entry);
                ksnTCPProxyServerClientDisconnect(tp, *fd);
            }
        }
        
        // Stop the server
    }
}

//! \file   tcp_proxy.c
//!     * Connect client to TCP Proxy server: ksnTCPProxyServerClientConnect()
//!         * Register connection 
//!         * Start UDP port Proxy
//!         * Start resending datagrams between TCP Server and UDP port Proxy 
/**
 * Connect TCP Proxy Server client
 * 
 * Called when tcp client connected to TCP Proxy Server. Create UDP Proxy and 
 * register this client in TCP Proxy map
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * @param fd TCP client connection file descriptor
 * 
 */
void ksnTCPProxyServerClientConnect(ksnTCPProxyClass *tp, int fd) {
    
    int udp_proxy_fd, udp_proxy_port = kev->ksn_cfg.port;
    
    ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s TCP Proxy client fd %d connected\n", 
            ANSI_YELLOW, ANSI_NONE, fd);
   
    // Open UDP Proxy client/server
    udp_proxy_fd = ksnCoreBindRaw(&kev->ksn_cfg, &udp_proxy_port);
    ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s UDP client/server Proxy created at port %d\n", 
            ANSI_YELLOW, ANSI_NONE,
            udp_proxy_port);
            
    // Register client in tcp proxy map 
    ksnTCPProxyData data;
    data.udp_proxy_fd = udp_proxy_fd;
    data.udp_proxy_port = udp_proxy_port;
    pblMapAdd(tp->map, &fd, sizeof(fd), &data, sizeof(ksnTCPProxyData));
    
    // Create and start watcher (start client processing)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"
    size_t valueLength;
    ksnTCPProxyData* tpd = pblMapGet(tp->map, &fd, sizeof(fd), &valueLength);
    if(tpd != NULL) {        
        ev_init (&tpd->w, cmd_tcpp_read_cb);
        ev_io_set (&tpd->w, fd, EV_READ);
        tpd->w.data = tp;
        ev_io_start (kev->ev_loop, &tpd->w);
    }
    #pragma GCC diagnostic pop
}

//! \file   tcp_proxy.c
//!     * Disconnect client from TCP Proxy server: ksnTCPProxyServerClientDisconnect()
/**
 * Disconnect TCP Proxy Server client
 * 
 * Called when client disconnected or when the TCP Proxy server closing. Close 
 * UDP and TCP connections, remove data from TCP Proxy clients map.
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * @param fd TCP client connection file descriptor
 */
void ksnTCPProxyServerClientDisconnect(ksnTCPProxyClass *tp, int fd) {
    
        size_t valueLength;
        
        // Get data from TCP Proxy Clients map, close TCP watcher and UDP PRoxy
        // connection and remove this data record from map
        ksnTCPProxyData* tpd = pblMapGet(tp->map, &fd, sizeof(fd), &valueLength); 
        if(tpd != NULL) {
            ev_io_stop (kev->ev_loop, &tpd->w); // Stop TCP Proxy client watcher
            close(tpd->udp_proxy_fd); // Close UDP Proxy connection          
            pblMapRemove(tp->map, &fd, sizeof(fd), &valueLength); // Remove data from map
        }
        
        // Close TCP Proxy client        
        close(fd); 
        
        // Show disconnect message
        ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s TCP Proxy client fd %d disconnected\n", 
            ANSI_YELLOW, ANSI_NONE, fd);
}

#undef kev
