/** 
 * \file   tcp_proxy.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet TCP Proxy module
 * 
 * See example: @ref teotproxy.c
 * 
 * See test: test_tcp_proxy.c
 *
 * Created on September 8, 2015, 1:59 AM
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

// Client functions -----------------------------------------------------------

// Server functions -----------------------------------------------------------