/** 
 * \file   tcp_proxy.h
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet TCP Proxy module
 *
 * Created on September 8, 2015, 2:00 AM
 */

#ifndef TCP_PROXY_H
#define	TCP_PROXY_H

#include <stdio.h>
#include <pbl.h>
#include <ev.h>

/**
 * TCP Proxy class data
 */
typedef struct ksnTCPProxyClass {
    
    void *ke;    ///< Pointer to ksnetEvMgrClass
    int fd;      ///< TCP Server fd or 0 if not started
    PblMap* map; ///< Hash Map to store tcp proxy client connections
    
} ksnTCPProxyClass;

/**
 * ksnTCPProxyClass map data
 */
typedef struct ksnTCPProxyData {
    
    int udp_proxy_port; ///< UDP Proxy port number
    int udp_proxy_fd;   ///< UDP Proxy file descriptor
    ev_io w;            ///< TCP Client watcher
    
} ksnTCPProxyData;

#ifdef	__cplusplus
extern "C" {
#endif

ksnTCPProxyClass *ksnTCPProxyInit(void *ke);
void ksnTCPProxyDestroy(ksnTCPProxyClass *tp);

#ifdef	__cplusplus
}
#endif

#endif	/* TCP_PROXY_H */
