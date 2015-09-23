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
    
    WAIT_FOR_START, ///< Wait for begibbibg of the packet
    WAIT_FOR_END, ///< Wait for end of the packet
    PROCESS_PACKET ///< Process the packet in buffer
    
} ksnTCPProxyBufferStage;

/**
 * TCP Proxy protocol command
 */
typedef enum {
    
    CMD_TCPP_PROXY  ///< Resend packet to UDP Proxy client/server
    
} ksnTCPProxyCommand;

/**
 * TCP Proxy packet(message) header structure
 */
typedef struct ksnTCPProxyHeader {
    
    uint8_t checksum; ///< Whole checksum
    unsigned int version : 4; ///< Protocol version number    
    unsigned int command : 4; ///< Protocol command
    uint16_t port; ///< UDP peers port number
    uint16_t packet_length; ///< UDP packet length
    uint8_t addr_length; ///< UDP peers address string length included trailing zero  
    uint8_t packet_checksum; ///< Header checksum
    
} ksnTCPProxyHeader;

/**
 * TCP Proxy packet data structure
 */
typedef struct ksnTCPProxyPacketData {
    
    char buffer[KSN_BUFFER_DB_SIZE]; ///< Packet buffer
    ksnTCPProxyHeader* header; ///< Packet header
    size_t length; ///< Received package length
    size_t ptr;  ///< Pointer to data end in packet buffer
    int stage;   ///< Packet buffer receiving stage    
    
} ksnTCPProxyPacketData;

/**
 * ksnTCPProxyClass map data
 */
typedef struct ksnTCPProxyData {
    
    int tcp_proxy_fd;   ///< TCP Proxy file descriptor
    int udp_proxy_port; ///< UDP Proxy port number
    int udp_proxy_fd;   ///< UDP Proxy file descriptor
    ev_io w;            ///< TCP Client watcher
    ev_io w_udp;        ///< UDP Client watcher
    ksnTCPProxyPacketData packet; ///< Packet buffer
    
} ksnTCPProxyData;

/**
 * TCP Proxy class data
 */
typedef struct ksnTCPProxyClass {
    
    void *ke;       ///< Pointer to ksnetEvMgrClass
    
    // Client
    int fd_client;  ///< TCP Client fd or 0 if not started (or not connected)
    ev_io w_client; ///< TCP Client watcher
    ksnTCPProxyPacketData packet; ///< TCP Client Packet buffer
    
    // Server
    int fd;         ///< TCP Server fd or 0 if not started
    PblMap* map;    ///< Hash Map to store tcp proxy client connections       
    
} ksnTCPProxyClass;

#ifdef	__cplusplus
extern "C" {
#endif

ksnTCPProxyClass *ksnTCPProxyInit(void *ke);
void ksnTCPProxyDestroy(ksnTCPProxyClass *tp);

int ksnTCPProxyClientConnetc(ksnTCPProxyClass *tp);

#ifdef	__cplusplus
}
#endif

#endif	/* TCP_PROXY_H */
