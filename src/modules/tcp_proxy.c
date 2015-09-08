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
 * Start TCP Proxy server
 * 
 * @param tp
 * @return 
 */
int ksnTCPProxyServerStart(ksnTCPProxyClass *tp) {
    
    // \todo
    
    return 0;
}

//! \file   tcp_proxy.c
//!     * When client is connected to TCP Proxy server: 
//!
//! \file   tcp_proxy.c
//!         * Register connection 
//!         * Start UDP port Proxy
//!         * Resend datagrams between TCP Server and UDP port Proxy 

