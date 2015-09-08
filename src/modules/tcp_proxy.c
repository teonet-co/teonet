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
#include "utils/rlutil.h"

int ksnTCPProxyServerStart(ksnTCPProxyClass *tp);

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
        
        free(tp);
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

//! \file   tcp_proxy.c
//!     * Start TCP Proxy server

/**
 * TCP Proxy server accept callback
 * 
 * @param loop Event manager loop
 * @param watcher Pointer to watcher
 * @param revents Events
 * @param fd File description of created client connection
 */
void ksn_tcpp_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *watcher,
                       int revents, int fd) {
    
    // \todo
    printf("TCP Proxy client connected\n");
}

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
                ksn_tcpp_accept_cb, 
                NULL, 
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
//!     * When client is connected to TCP Proxy server: 
//!
//! \file   tcp_proxy.c
//!         * Register connection 
//!         * Start UDP port Proxy
//!         * Resend datagrams between TCP Server and UDP port Proxy 

#undef kev
