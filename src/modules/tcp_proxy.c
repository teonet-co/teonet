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
#include <string.h>
#include <netinet/tcp.h>

#include "tcp_proxy.h"

#include "ev_mgr.h"
#include "net_core.h"
#include "tr-udp_.h"
#include "utils/rlutil.h"

// Core module functions
void host_cb(EV_P_ ev_io *w, int revents);
int send_cmd_disconnect_cb(ksnetArpClass *ka, char *name,
                            ksnet_arp_data *arp_data, void *data);

// Local function definition
void _cmd_tcpp_read_cb(struct ev_loop *loop, struct ev_io *w, int revents, 
        int cli_ser);
//
void ksnTCPProxyClientStop(ksnTCPProxyClass *tp);
//
size_t ksnTCPProxyPackageCreate(void *buffer, size_t buffer_length, 
        const char *addr, int port, int cmd, 
        const void *data, size_t data_length);
//
int ksnTCPProxyServerStart(ksnTCPProxyClass *tp);
void ksnTCPProxyServerStop(ksnTCPProxyClass *tp);
void ksnTCPProxyServerClientConnect(ksnTCPProxyClass *tp, int fd);
void ksnTCPProxyServerClientDisconnect(ksnTCPProxyClass *tp, int fd, 
        int remove_f);
//
ssize_t ksnTCPProxySendTo(ksnetEvMgrClass* ke, int cmd, const void *buffer, 
        size_t buffer_len, __CONST_SOCKADDR_ARG addr);

/**
 * Pointer to ksnetEvMgrClass
 * 
 */
#define kev ((ksnetEvMgrClass*)tp->ke)

#define TCP_PROXY_VERSION 0 ///< TCP Proxy version

enum ksnTCPProxyCliSer {
    
    CLIENT,
    SERVER
            
} ;

// Initialize / Destroy functions ---------------------------------------------

/**
 * Initialize TCP Proxy module 
 * 
 * @param ke Pointer to the ksnTCPProxyClass
 * 
 * @return Pointer to ksnTCPProxyClass
 */
ksnTCPProxyClass *ksnTCPProxyInit(void *ke) {
    
    ksnTCPProxyClass *tp = malloc(sizeof(ksnTCPProxyClass));
    tp->map = pblMapNewHashMap();
    tp->ke = ke;
    tp->fd = 0;  
    tp->fd_client = 0; 
    
    // Start TCP proxy client
    ksnTCPProxyClientConnetc(tp);
    
    // Start TCP proxy server
    ksnTCPProxyServerStart(tp);
    
    return tp;
}

/**
 * Destroy TCP Proxy module
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * 
 */
void ksnTCPProxyDestroy(ksnTCPProxyClass *tp) {
    
    if(tp != NULL) {
        ksnTCPProxyClientStop(tp); // Stop TCP Proxy client
        ksnTCPProxyServerStop(tp); // Stop TCP Proxy server
        pblMapFree(tp->map); // Free clients map
        free(tp); // Free class memory
    }
}

// Common functions -----------------------------------------------------------

/**
 * Receive packet from UDP or TCP Proxy buffer
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @param fd
 * @param buffer
 * @param buffer_len
 * @param flags
 * @param addr
 * @param addr_len
 * 
 * @return 
 * @retval >0 - Success
 * @retval -2 - Buffer too small error
 * @retval -3 - Wrong address error
 */
ssize_t teo_recvfrom (ksnetEvMgrClass* ke, 
            int fd, void *buffer, size_t buffer_len, int flags,
            __SOCKADDR_ARG addr, socklen_t *__restrict addr_len) {
    
    ssize_t recvlen = 0; 
    
    // Get data from TCP Proxy buffer 
    if(!fd && ke->ksn_cfg.r_tcp_f && ke->tp->fd_client > 0) {
        
        if(buffer_len >= ke->tp->packet.header->packet_length) {

            // Copy data to buffer      
            memcpy(
                buffer, 
                ke->tp->packet.buffer + 
                    sizeof(ksnTCPProxyHeader) + 
                    ke->tp->packet.header->addr_length, 
                ke->tp->packet.header->packet_length
            );

            // Make address from string
            if(!ksnTRUDPmakeAddr(
                    ke->tp->packet.buffer + sizeof(ksnTCPProxyHeader), 
                    ke->tp->packet.header->port, 
                    addr, addr_len)) {

                recvlen = ke->tp->packet.header->packet_length;
            }
            else recvlen = -3; // Wrong address error
        }
        else recvlen = -2; // Buffer too small error
    } 
    
    // Get data from UDP 
    else if(fd) {  
        
        recvlen = recvfrom(fd, buffer, buffer_len, flags, addr, addr_len);
    }
    
    return recvlen;
}

/**
 * Send UDP packet by UDP or TCP Proxy
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @param fd
 * @param buffer
 * @param buffer_len
 * @param flags
 * @param addr
 * @param addr_len
 * 
 * @return 
 */
ssize_t teo_sendto (ksnetEvMgrClass* ke,
            int fd, const void *buffer, size_t buffer_len, int flags, 
            __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {
    
    ssize_t sendlen = 0;
    
    // Sent data to TCP Proxy
    if(ke->ksn_cfg.r_tcp_f && ke->tp->fd_client > 0) {
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
            "%sTCP Proxy:%s "
            "Send %d bytes to TCP Proxy server, fd %d\n", 
            ANSI_YELLOW, ANSI_NONE, buffer_len, ke->tp->fd_client);
        #endif            
        
        // Send TCP package
        sendlen = ksnTCPProxySendTo(ke, CMD_TCPP_PROXY, buffer, buffer_len, addr);
    }
    
    // Sent data to UDP
    else sendlen = sendto(fd, buffer, buffer_len, flags, addr, addr_len);
    
    return sendlen;
}

/**
 * Send command to TCP Proxy
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @param cmd TCP Proxy command
 * @param buffer
 * @param buffer_len
 * @param addr
 * 
 * @return 
 */
ssize_t ksnTCPProxySendTo(ksnetEvMgrClass* ke,
            int cmd, const void *buffer, size_t buffer_len,  
            __CONST_SOCKADDR_ARG addr) {
    
    ssize_t sendlen = 0;
    
    // Create TCP package
    const size_t tcp_buffer_len = KSN_BUFFER_DB_SIZE;
    char tcp_buffer[tcp_buffer_len];
    size_t pl = ksnTCPProxyPackageCreate(
        tcp_buffer, tcp_buffer_len, 
        inet_ntoa(((struct sockaddr_in*)addr)->sin_addr),
        ntohs(((struct sockaddr_in*)addr)->sin_port),
        cmd,
        buffer, buffer_len
    );

    // Send TCP package
    sendlen = write(ke->tp->fd_client, tcp_buffer, pl);
        
    return sendlen;
}

/**
 * Set TCP socket NODELAY option
 * 
 * @param ke Pointer toksnetEvMgrClass
 * @param fd TCP socket descriptor
 * 
 * @return Result of setting. Success if >= 0.
 */
int set_tcp_nodelay(ksnetEvMgrClass* ke, int fd) {

    int flag = 1;
    int result = setsockopt(fd,           // socket affected
                         IPPROTO_TCP,     // set option at TCP level
                         TCP_NODELAY,     // name of option
                         (char *) &flag,  // the cast is historical cruft
                         sizeof(int));    // length of option value
    if (result < 0) {
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG, 
            "%sTCP Proxy:%s "
            "Set TCP_NODELAY of fd %d error %s\n", 
            ANSI_YELLOW, ANSI_RED, fd, ANSI_NONE);
        #endif
    }
    
    return result;
}

/**
 * Calculate checksum
 * 
 * Calculate byte checksum in data buffer
 * 
 * @param data Pointer to data buffer
 * @param data_length Length of the data buffer to calculate checksum
 * 
 * @return Byte checksum of the input buffer
 */
uint8_t ksnTCPProxyChecksumCalculate(void *data, size_t data_length) {
    
    int i;
    uint8_t *ch, checksum = 0;
    for(i = 0; i < data_length; i++) {
        ch = (uint8_t*)(data + i);
        checksum += *ch;
    }
    
    return checksum;
}

/**
 * Create TCP Proxy package
 * 
 * Create TCP Proxy package from peers UDP address and port, data buffer and 
 * its length
 * 
 * @param buffer The buffer to create package in
 * @param buffer_length Package data length
 * @param addr String with peer UDP address
 * @param port UDP port number
 * @param cmd TCP Proxy protocol command
 * @param data Package data
 * @param data_length Package data length
 * 
 * @return Return size of created package or error
 *  
 * @retval > 0 - size of created package
 * @retval -1 - error: The output buffer less than packet header
 * @retval -2 - error: The output buffer less than packet header + data
 */
size_t ksnTCPProxyPackageCreate(void *buffer, size_t buffer_length, 
        const char *addr, int port, int cmd, 
        const void *data, size_t data_length) {
    
    size_t tcp_package_length;
    
    if(buffer_length >= sizeof(ksnTCPProxyHeader)) {
        
        ksnTCPProxyHeader *th = (ksnTCPProxyHeader*)buffer;

        th->version = TCP_PROXY_VERSION; // TCP Proxy protocol version 
        th->command = cmd; // CMD_TCPP_PROXY; // TCP Proxy protocol command
        th->addr_length = strlen(addr) + 1; // Address string length
        th->port = port; // UDP port number
        th->packet_length = data_length; // Package data length   
        th->packet_checksum = ksnTCPProxyChecksumCalculate((void*)th + 1, 
                sizeof(ksnTCPProxyHeader) - 2);
        
        size_t p_length = sizeof(ksnTCPProxyHeader) + th->addr_length + data_length;        
        if(buffer_length >= p_length) {
            
            tcp_package_length = p_length;
            memcpy(buffer + sizeof(ksnTCPProxyHeader), addr, th->addr_length); // Address string
            memcpy(buffer + sizeof(ksnTCPProxyHeader) + th->addr_length, data, data_length); // Package data        
            th->checksum = ksnTCPProxyChecksumCalculate(buffer + 1, tcp_package_length - 1); // Package data length
        } 
        else tcp_package_length = -2; // Error code: The output buffer less than packet data + header
    }
    else tcp_package_length = -1; // Error code: The output buffer less than packet header
    
    return tcp_package_length;
}

/**
 * Process TCP proxy package
 * 
 * Read tcp data from input buffer to tp->buffer until end of tcp proxy package, 
 * check checksum, take UDP address, port number and UDP package data.
 * 
 * @param packet Pointer to ksnTCPProxyPacketData
 * @param data Pointer to received TCP data
 * @param data_length TCP data length
 * 
 * @return Length of received packet, zero or error code. The TCP packets may 
 *         be received combined to one big buffer. If packet processed and this 
 *         function return value grate than 0 and less than input buffer size 
 *         we need run this function again with data_length = 0 to process next 
 *         part of input buffer. \n
 *         (see the "5) Check receiving" code of the test_5_2() function of the 
 *         test_tcp_proxy.c test)
 * 
 * @retval >0 - receiving done, the return value contain length of packet, 
 *              the packet saved to ksnTCPProxyClass::buffer
 * @retval 0 - continue reading current packet 
 * @retval -1 - wrong process package stage 
 * @retval -2 - wrong packet header checksum
 * @retval -3 - wrong packet checksum
 */
int ksnTCPProxyPackageProcess(ksnTCPProxyPacketData *packet, void *data, 
        size_t data_length) {
    
    int rv = 0;
    
    switch(packet->stage) {
        
        // Wait for package began
        case WAIT_FOR_START:
            if(packet->ptr != 0) {
                packet->ptr = packet->ptr - packet->length;
                memmove(packet->buffer, packet->buffer+packet->length, 
                        packet->ptr);
            }
            packet->length = 0;
            packet->stage = WAIT_FOR_END;
            rv = ksnTCPProxyPackageProcess(packet, data, data_length);
            break;
            
        // Wait for package end
        case WAIT_FOR_END: {
            
            // \todo check buffer length
            if(data_length > 0) {
                memcpy(packet->buffer + packet->ptr, data, data_length);  
                packet->ptr += data_length;
            }
            if(packet->ptr >= sizeof(ksnTCPProxyHeader)) {
                
                // Check packet header 
                uint8_t packet_checksum = 
                        ksnTCPProxyChecksumCalculate(
                            (void*)packet->header + 1, 
                            sizeof(ksnTCPProxyHeader) - 2
                        );
                
                // If packet checksum not equal - skip this packet
                if(!(packet->header->addr_length 
                     && packet->header->port     
                     && packet->header->packet_checksum == packet_checksum)) {  
                    
                    rv = -2;
                    packet->stage = WAIT_FOR_START;
                    packet->ptr = 0;
                    break;
                }
                
                size_t pkg_len = sizeof(ksnTCPProxyHeader) + 
                        packet->header->packet_length + 
                        packet->header->addr_length;
                
                if(packet->ptr >= pkg_len) {
                    packet->stage = PROCESS_PACKET;
                    rv = ksnTCPProxyPackageProcess(packet, NULL, 0);
                }
            }
        }
        break;
            
        // Read process body
        case PROCESS_PACKET: {
            
            packet->length = 
                    sizeof(ksnTCPProxyHeader) + 
                    packet->header->packet_length + 
                    packet->header->addr_length;
            
            // Get packet checksum 
            uint8_t checksum = 
                    ksnTCPProxyChecksumCalculate(
                        (void*)packet->buffer + 1, 
                        packet->length - 1
                    );
            
            packet->stage = WAIT_FOR_START;
            if(!(packet->ptr > packet->length)) 
                packet->ptr = 0;
            
            // Packet is processed
            if(packet->header->checksum == checksum) {
                      
                rv = packet->length;
            }
            
            // Wrong packet checksum error
            else rv = -3;
        }
        break;
            
        // Some wrong package processing stage
        default:
            rv = -1;
            break;
    }
            
    return rv;
}

//! \file   tcp_proxy.c
//! * Client functions: 

//! \file   tcp_proxy.c
//!     * Connect to TCP Proxy server: ksnTCPProxyConnetc()

/**
 * TCP Proxy client callback
 * 
 * Get packet from TCP Proxy server connection and resend it to read host callback
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * 
 */
void cmd_tcppc_read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
    
    // TCP Proxy client callback
    _cmd_tcpp_read_cb(loop, w, revents, CLIENT);
}

/**
 * Connect to TCP Proxy server
 * 
 * Get address and port from teonet configuration and connect to R-Host TCP 
 * Server 
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * 
 * @return 0 - Successfully connected
 */
int ksnTCPProxyClientConnetc(ksnTCPProxyClass *tp) {
   
    if(kev->ksn_cfg.r_tcp_f) {
        
        // Initialize client input packet buffer parameters
        tp->packet.ptr = 0; // Pointer to data end in packet buffer
        tp->packet.length = 0; // Length of received packet
        tp->packet.stage = WAIT_FOR_START; // Stage of receiving packet
        tp->packet.header = (ksnTCPProxyHeader*) tp->packet.buffer; // Pointer to packet header
                
        // Connect to R-Host TCP Server
        int fd_client = ksnTcpClientCreate(
                kev->kt, // Pointer to ksnTcpClass
                kev->ksn_cfg.r_tcp_port, // Remote host TCP port number
                kev->ksn_cfg.r_host_addr // Remote host net address
        );
        
        if(fd_client > 0) {
            
            // Set TCP_NODELAY option
            set_tcp_nodelay(kev, fd_client);

            // Register connection
            tp->fd_client = fd_client;

            // \todo TCP Proxy protocol connect
                        
            // Create and start TCP Proxy client watcher
            ev_init (&tp->w_client, cmd_tcppc_read_cb);
            ev_io_set (&tp->w_client, fd_client, EV_READ);
            tp->w_client.data = tp;
            ev_io_start (kev->ev_loop, &tp->w_client);
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&kev->ksn_cfg, DEBUG, 
                "%sTCP Proxy:%s "
                "TCP Proxy client fd %d started at port %d\n", 
                ANSI_YELLOW, ANSI_NONE, fd_client, kev->ksn_cfg.r_tcp_port);
            #endif
        }
    }
    
    return 0;
}

/**
 * Stop TCP Proxy client
 * 
 * Stop TCP Proxy client watcher and close fd connection
 * 
 * @param tp Pointer to ksnTCPProxyClass
 */
void ksnTCPProxyClientStop(ksnTCPProxyClass *tp) {
    
    if(kev->ksn_cfg.r_tcp_f) {
        
        if(tp->fd_client > 0) {
            
            ev_io_stop(kev->ev_loop, &tp->w_client);
            close(tp->fd_client);
            tp->fd_client = 0;
            
            // Remove ALL records from ARP table
            if(kev->kc != NULL) ksnetArpRemoveAll(kev->kc->ka);
        }
    }
}


//! \file   tcp_proxy.c
//! * Server functions: 

/**
 * UDP client/server Proxy callback
 * 
 * Get packet from UDP Proxy client connection and resend it to TCP Proxy
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * 
 */
void cmd_udpp_read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
    
    struct sockaddr_in remaddr; // Remote address
    socklen_t addrlen = sizeof(remaddr); // Length of addresses
    size_t data_len = KSN_BUFFER_DB_SIZE; // Buffer length
    ksnTCPProxyClass *tp = w->data; // Pointer to ksnTCPProxyClass
    char data[data_len]; // Buffer
    const int flags = 0; // Flags

    // Get UDP data
    ssize_t received = recvfrom(w->fd, data, data_len, flags, 
            (struct sockaddr *)&remaddr, &addrlen);
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTCP Proxy:%s "
            "Got something from UDP fd %d w->events = %d, received = %d ...\n", 
            ANSI_YELLOW, ANSI_NONE, w->fd, w->events, (int)received);
    #endif
  
    // Resend UDP packet to TCP Proxy
    if(received > 0) {
        
        size_t data_send_len = KSN_BUFFER_DB_SIZE; 
        char data_send[data_send_len];
        
        // Create TCP package
        size_t pl = ksnTCPProxyPackageCreate(data_send, data_send_len, 
                inet_ntoa(((struct sockaddr_in *) &remaddr)->sin_addr), 
                ntohs(((struct sockaddr_in *) &remaddr)->sin_port), 
                CMD_TCPP_PROXY, 
                data, received
        );
        
        // Get TCP fd from tcp proxy map
        size_t valueLength;
        ksnTCPProxyData* tpd = pblMapGet(tp->map, &w->fd, sizeof(w->fd), 
                &valueLength);
        if(tpd != NULL) {   
        
            // Send TCP package
            ssize_t rv = write(tpd->tcp_proxy_fd, data_send, pl);
            
            // Check write to TCP Proxy client error
            if(rv <= 0 || pl <= 0 || rv != pl) {
                
                // Show write to TCP Proxy client error
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG,
                    "%sTCP Proxy:%s "
                    "Send to TCP client fd %d error, rv: %d, pl: %d %s\n", 
                    ANSI_YELLOW, ANSI_RED, tpd->tcp_proxy_fd, rv, pl, ANSI_NONE);
                #endif
            } 
            
            // Show successfully write message
            else {
                
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                    "%sTCP Proxy:%s "
                    "Resent UDP packet to TCP client, packet size: %d bytes\n", 
                    ANSI_YELLOW, ANSI_NONE, pl);
                #endif
            }
        }
        else {
            
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG, 
                    "%sTCP Proxy:%s "
                    "Can't find fd %d in TCP Proxy map%s\n", 
                    ANSI_YELLOW, ANSI_RED, w->fd, ANSI_NONE);
                #endif
        }
    }    
}

/**
 * TCP Proxy client or server client callback
 * 
 * Get packet from TCP Proxy server client connection and resend it to UDP Proxy
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * @param cli_ser
 * 
 */
void _cmd_tcpp_read_cb(struct ev_loop *loop, struct ev_io *w, int revents, 
        int cli_ser) {

    size_t data_len = KSN_BUFFER_SIZE; // Buffer length
    ksnTCPProxyClass *tp = w->data; // Pointer to ksnTCPProxyClass
    char data[data_len]; // Buffer
    
    // Read TCP data
    ssize_t received = read(w->fd, data, data_len);
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
            "%sTCP Proxy:%s "
            "Got something from fd %d w->events = %d, received = %d ...\n", 
            ANSI_YELLOW, ANSI_NONE, w->fd, w->events, (int)received);
    #endif

    // Disconnect client:
    // Close UDP and TCP connections and Remove data from TCP Proxy Clients map
    if(!received) {        
        
        #ifdef DEBUG_KSNET
        ksnet_printf(
            &kev->ksn_cfg , DEBUG,
            "%sTCP Proxy:%s "
            "Connection closed. Stop listening fd %d ...\n",
            ANSI_YELLOW, ANSI_NONE, w->fd
        );
        #endif
        
        if(cli_ser == SERVER)
            ksnTCPProxyServerClientDisconnect(tp, w->fd, 1);
        else 
            ksnTCPProxyClientStop(tp);
    } 
    
    // \todo Process reading error
    else if (received < 0) {
        
        //        if( errno == EINTR ) {
        //            // OK, just skip it
        //        }
    
        #ifdef DEBUG_KSNET
        ksnet_printf(
            &kev->ksn_cfg, DEBUG,
            "%sTCP Proxy:%s Read error ...%s\n", 
            ANSI_YELLOW, ANSI_RED, ANSI_NONE
        );
        #endif
    }
    
    // Success read. Process package and resend it to UDP proxy when ready
    else {
        
        // Get TCP fd from tcp proxy map
        ksnTCPProxyData* tpd;
        ksnTCPProxyPacketData *packet;
        if(cli_ser == SERVER) {
            size_t valueLength;
            tpd = pblMapGet(tp->map, &w->fd, sizeof(w->fd), &valueLength);
            packet = &tpd->packet;
        }
        else {
            ksnTCPProxyData _tpd;
            tpd = &_tpd;
            packet = &tp->packet;
        }

        // Process received buffer
        if(tpd != NULL) for(;;) {
            
            // Parse TCP packet        
            int rv = ksnTCPProxyPackageProcess(packet, data, received);

            // Send package to peer by UDP proxy connection
            if(rv > 0) {

                // Address string
                const char *addr = (const char *) (packet->buffer + 
                    sizeof(ksnTCPProxyHeader));

                // Port number
                const int port = packet->header->port;

                // Pointer to packet        
                const char *packet_data = (const char *) (packet->buffer + 
                    sizeof(ksnTCPProxyHeader) + packet->header->addr_length);

                // Packet length
                const size_t packet_data_len = packet->header->packet_length;

                // Make address from string
                struct sockaddr_in remaddr; // remote address
                socklen_t addrlen = sizeof(remaddr); // length of addresses 
                if(!ksnTRUDPmakeAddr(addr, port, (__SOCKADDR_ARG) &remaddr, 
                        &addrlen)) {

                    // Execute TCP Proxy packet command
                    switch(packet->header->command) {

                        // Resend TCP packet to Peer by UDP proxy connection
                        case CMD_TCPP_PROXY: {

                            // Send TCP package to UDP Proxy
                            if(cli_ser == SERVER) {
                                sendto(tpd->udp_proxy_fd, 
                                       packet_data, packet_data_len, 0, 
                                       (__CONST_SOCKADDR_ARG) &remaddr, addrlen
                                ); 
                            }
                            
                            // Call host callback
                            else {
                                
                                // Process received TCP packet
                                void *data = w->data;
                                w->data = kev->kc;
                                host_cb(loop, w, EV_NONE);
                                w->data = data;
                            }

                        } break;
                        
//                        // Get host IPs list
//                        case CMD_TCPP_GET_IPS: {
//                            printf("\"Get host IPs list\" TCP Proxy command\n");
//                        } break;
//
//                        // Get host UDP proxy port
//                        case CMD_TCPP_GET_PORT: {
//                            printf("\"Get host UDP proxy port\" TCP Proxy command\n");
//                        } break;

                        default:
                            break;
                    }                
                }

                // Process next part of received buffer
                if(!packet->ptr == 0) {

                    received = 0;
                    continue; // Continue receiving buffer loop
                }
            } 

            // Process packed errors
            else if(rv < 0) {

                // Wrong process package stage error
                if(rv == -1) { 
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG, 
                        "%sTCP Proxy:%s "
                        "Wrong process package stage ...%s\n", 
                        ANSI_YELLOW, ANSI_RED, ANSI_NONE);
                    #endif
                }

                // Wrong packet received error
                else if(rv == -2) {
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG, 
                        "%sTCP Proxy:%s "
                        "Wrong packet received ...%s\n", 
                        ANSI_YELLOW, ANSI_RED, ANSI_NONE);
                    #endif
                }

                // Wrong packet body received error
                else if(rv == -3) {
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG, 
                        "%sTCP Proxy:%s "
                        "Wrong packet checksum ...%s\n", 
                        ANSI_YELLOW, ANSI_RED, ANSI_NONE);
                    #endif
                }
                
                // Close TCP Proxy client
                ksnTCPProxyServerClientDisconnect(tp, w->fd, 1);
            }

            // A part of packet received - continue receiving next parts of package
            else {                
                // Do nothing
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                    "%sTCP Proxy:%s "
                    "Part of packet received. Wait for next part ...%s\n", 
                    ANSI_YELLOW, ANSI_GREY, ANSI_NONE);
                #endif                
            }
            
            break; // Exit from process received buffer loop
        }
    }
}

/**
 * TCP Proxy server client callback
 * 
 * Get packet from TCP Proxy server client connection and resend it to UDP Proxy
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * 
 */
void cmd_tcpp_read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
    
    // TCP Proxy server callback
    _cmd_tcpp_read_cb(loop, w, revents, SERVER);
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
 * 
 */
void cmd_tcpp_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *w,
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
 * 
 * @return If return value > 0 than server was created successfully
 */
int ksnTCPProxyServerStart(ksnTCPProxyClass *tp) {
    
    int fd = 0;
    
    if(kev->ksn_cfg.tcp_allow_f) {
        
        // Create TCP server at port, which will wait client connections
        int port_created;
        if((fd = ksnTcpServerCreate(
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
 * 
 */
void ksnTCPProxyServerStop(ksnTCPProxyClass *tp) {
    
    // If server started
    if(kev->ksn_cfg.tcp_allow_f && tp->fd) {
        
        // Disconnect all clients
        PblIterator *it = pblMapIteratorReverseNew(tp->map);
        if(it != NULL) {
            while(pblIteratorHasPrevious(it) > 0) {
                void *entry = pblIteratorPrevious(it);
                int *fd = (int *) pblMapEntryKey(entry);
                ksnTCPProxyServerClientDisconnect(tp, *fd, 0);
            }
            pblIteratorFree(it);
        }
                
        // Clear map
        pblMapClear(tp->map);
        
        // \todo Stop the server
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
    
    // Set TCP_NODELAY option
    set_tcp_nodelay(kev, fd);

    int udp_proxy_fd, udp_proxy_port = kev->ksn_cfg.port;
    
    ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s "
            "TCP Proxy client fd %d connected\n", 
            ANSI_YELLOW, ANSI_NONE, fd);
   
    // Open UDP Proxy client/server
    ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s "
            "Create UDP client/server Proxy at port %d ...\n", 
            ANSI_YELLOW, ANSI_NONE,
            udp_proxy_port);
    udp_proxy_fd = ksnCoreBindRaw(&kev->ksn_cfg, &udp_proxy_port);
    ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s "
            "UDP client/server Proxy fd %d created at port %d\n", 
            ANSI_YELLOW, ANSI_NONE,
            udp_proxy_fd,
            udp_proxy_port);
            
    // Register client in tcp proxy map 
    ksnTCPProxyData data;
    data.tcp_proxy_fd = fd;
    data.udp_proxy_fd = udp_proxy_fd;
    data.udp_proxy_port = udp_proxy_port;
    pblMapAdd(tp->map, &fd, sizeof(fd), &data, sizeof(ksnTCPProxyData));
    pblMapAdd(tp->map, &udp_proxy_fd, sizeof(udp_proxy_fd), &data, sizeof(ksnTCPProxyData));
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"

    // Create and start TCP and UDP watchers (start client processing)
    size_t valueLength;
    ksnTCPProxyData* tpd = pblMapGet(tp->map, &fd, sizeof(fd), &valueLength);
    if(tpd != NULL) {   
        
        // Initialize input packet buffer parameters
        tpd->packet.ptr = 0; // Pointer to data end in packet buffer
        tpd->packet.length = 0; // Length of received packet
        tpd->packet.stage = WAIT_FOR_START; // Stage of receiving packet
        tpd->packet.header = (ksnTCPProxyHeader*) tpd->packet.buffer; // Pointer to packet header
        
        // Create and start TCP watcher (start TCP client processing)
        ev_init (&tpd->w, cmd_tcpp_read_cb);
        ev_io_set (&tpd->w, fd, EV_READ);
        tpd->w.data = tp;
        ev_io_start (kev->ev_loop, &tpd->w);
        
        // Create and start UDP watcher (start UDP client processing)
        ev_init (&tpd->w_udp, cmd_udpp_read_cb);
        ev_io_set (&tpd->w_udp, udp_proxy_fd, EV_READ);
        tpd->w_udp.data = tp;
        ev_io_start (kev->ev_loop, &tpd->w_udp);
    }
    
    // Error: can't register TCP fd in tcp proxy map
    else {
        // \todo process error: can't register TCP fd in tcp proxy map
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
 * @param remove_f If true than remove  disconnected record from map
 * 
 */
void ksnTCPProxyServerClientDisconnect(ksnTCPProxyClass *tp, int fd, 
        int remove_f) {

    size_t valueLength;

    // Get data from TCP Proxy Clients map, close TCP watcher and UDP PRoxy
    // connection and remove this data record from map
    ksnTCPProxyData* tpd = pblMapGet(tp->map, &fd, sizeof(fd), &valueLength); 
    if(tpd != NULL) {

        // Skip UDP Proxy fd record
        if(fd == tpd->udp_proxy_fd) return;
        
        // Stop TCP Proxy client watcher
        ev_io_stop (kev->ev_loop, &tpd->w);
        close(fd); 

        // Show disconnect message
        ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s TCP Proxy client fd %d disconnected\n", 
            ANSI_YELLOW, ANSI_NONE, fd);

        // Stop UDP client/server Proxy watcher and Close UDP Proxy connection 
        ev_io_stop (kev->ev_loop, &tpd->w_udp);
        close(tpd->udp_proxy_fd); 

        // Show disconnect message
        ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s UDP Proxy client fd %d disconnected\n", 
            ANSI_YELLOW, ANSI_NONE, tpd->udp_proxy_fd);

        // Remove data from map
        if(remove_f) {
            pblMapRemove(tp->map, &tpd->udp_proxy_fd, sizeof(tpd->udp_proxy_fd), &valueLength);
            pblMapRemove(tp->map, &fd, sizeof(fd), &valueLength);
        }
        
        // \todo Send disconnect command to all peers
//        char *peer_name = "teovpn-c4";
//        ksnetArpGetAll(kev->kc->ka, send_cmd_disconnect_cb, peer_name);
        
        // \todo Remove this client connection from ARP table !!! Don't need because it removed without it
//        ksnCorePacketData rd;
//        rd.from = peer_name;
//        cmd_disconnected_cb(kev->kc->kco, &rd);
    }        
}

#undef kev
