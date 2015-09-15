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

#include <stdint.h>
#include <stdio.h>
#include <pbl.h>
#include <ev.h>

#include "config/conf.h"

/**
 * TCP Proxy buffer stage
 */
typedef enum {
    
    WAIT_FOR_BEGAN, ///< Wait for packet beginning
    WAIT_FOR_END, ///< Wait for end of packet
    PROCESS_PACKET ///< Process the packet in buffer
    
} ksnTCPProxyBufferStage;

/**
 * TCP Proxy packet(message) header structure
 */
typedef struct ksnTCPProxyMessage_header {
    
    uint8_t checksum; ///< Checksum
    unsigned int version : 4; ///< Protocol version number    
    unsigned int addr_len : 4; ///< UDP peers address string length included trailing zero  
    uint16_t port; ///< UDP peers port number
    uint16_t package_len; ///< UDP package length
    uint16_t reserved; ///< Reserved for funure use
    
} ksnTCPProxyMessage_header;

/**
 * TCP Proxy class data
 */
typedef struct ksnTCPProxyClass {
    
    void *ke;    ///< Pointer to ksnetEvMgrClass
    int fd;      ///< TCP Server fd or 0 if not started
    PblMap* map; ///< Hash Map to store tcp proxy client connections
    char *buffer[KSN_BUFFER_DB_SIZE]; ///< TCP Proxy packet buffer
    size_t ptr;  ///< TCP Proxy packet buffer
    int stage;   ///< TCP Proxy packet buffer
    
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
