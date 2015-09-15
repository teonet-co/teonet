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

#include "tcp_proxy.h"
#include "ev_mgr.h"
#include "net_core.h"
#include "utils/rlutil.h"

// Local function definition
int ksnTCPProxyServerStart(ksnTCPProxyClass *tp);
void ksnTCPProxyServerStop(ksnTCPProxyClass *tp);
void ksnTCPProxyServerClientConnect(ksnTCPProxyClass *tp, int fd);
void ksnTCPProxyServerClientDisconnect(ksnTCPProxyClass *tp, int fd, 
        int remove_f);
int ksnTCPProxyPackageProcess(ksnTCPProxyClass *tp, void *data, size_t data_len);

#define kev ((ksnetEvMgrClass*)tp->ke)

#define TCP_PROXY_VERSION 0

// Initialize / Destroy functions ---------------------------------------------

/**
 * Initialize TCP Proxy module 
 * 
 * @param ke Pointer to the 
 * 
 * @return Pointer to ksnTCPProxyClass
 */
ksnTCPProxyClass *ksnTCPProxyInit(void *ke) {
    
    ksnTCPProxyClass *tp = malloc(sizeof(ksnTCPProxyClass));
    tp->map = pblMapNewHashMap();
    tp->ke = ke;
    tp->fd = 0;  
    tp->ptr = 0;
    tp->stage = WAIT_FOR_BEGAN;
    
    ksnTCPProxyServerStart(tp);
    
    return tp;
}

/**
 * Destroy TCP Proxy module
 * 
 * @param tp
 * 
 */
void ksnTCPProxyDestroy(ksnTCPProxyClass *tp) {
    
    if(tp != NULL) {
        ksnTCPProxyServerStop(tp); // Stop TCP Proxy server
        pblMapFree(tp->map); // Free clients map
        free(tp); // Free class memory
    }
}

// Common functions -----------------------------------------------------------

/**
 * Calculate checksum
 * 
 * Calculate byte checksum in data buffer
 * 
 * @param data Data buffer
 * @param data_len Length of the data buffer
 * 
 * @return Byte checksum of buffer
 */
uint8_t ksnTCPProxyChecksumCalculate(void *data, size_t data_len) {
    
    int i;
    uint8_t *ch, checksum = 0;
    for(i = 1; i < data_len; i++) {
        ch = (uint8_t*)(data + i);
        checksum += *ch;
    }
    
    return checksum;
}

//! \file   tcp_proxy.c
//! * Client functions: 

//! \file   tcp_proxy.c
//!     * Connect to TCP Proxy server: ksnTCPProxyConnetc()

/**
 * Connect to TCP Proxy server
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * 
 * @return 0 - Successfully connected
 */
int ksnTCPProxyConnetc(ksnTCPProxyClass *tp) {
   
    // Get address and port from teonet config
    
    // Connect to R-Host TCP Server
    
    // Register connection (in ARP ?)
    
    // \todo ksnTCPProxyConnetc
    
    return 0;
}

/**
 * Create TCP Proxy package
 * 
 * Create TCP Proxy package from peers UDP address and port, data buffer and 
 * its length
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * @param buffer The buffer to create package in
 * @param buffer_length Package data length
 * @param addr String with peer UDP address
 * @param port UDP port number
 * @param data Package data
 * @param data_length Package data length
 * 
 * @return > 0 - Size of created package; 
 *         -1 - error: The output buffer less than packet header;
 *         -2 - error: The output buffer less than packet header + data
 */
size_t ksnTCPProxyPackageCreate(ksnTCPProxyClass *tp, void *buffer, 
        size_t buffer_length, const char *addr, int port, const void *data, 
        size_t data_length) { 
    
    size_t tcp_package_length;
    
    if(buffer_length >= sizeof(ksnTCPProxyMessage_header)) {
        
        ksnTCPProxyMessage_header *th = (ksnTCPProxyMessage_header*)buffer;

        th->version = TCP_PROXY_VERSION; // TCP Proxy protocol version 
        th->addr_len = strlen(addr) + 1; // Address string length
        th->port = port; // UDP port number
        th->package_len = data_length; // Package data length   
        size_t p_length = sizeof(ksnTCPProxyMessage_header) + th->addr_len + data_length;
        
        if(buffer_length >= p_length) {
            
            tcp_package_length = p_length;
            memcpy(buffer + sizeof(ksnTCPProxyMessage_header), addr, th->addr_len); // Address string
            memcpy(buffer + sizeof(ksnTCPProxyMessage_header) + th->addr_len, data, data_length); // Package data        
            th->checksum = ksnTCPProxyChecksumCalculate(buffer, tcp_package_length); // Package data length
        } 
        else tcp_package_length = -2; // Error code: The output buffer less than packet data + header
    }
    else tcp_package_length = -1; // Error code: The output buffer less than packet header
    
    return tcp_package_length;
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
 * 
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
        
        #ifdef DEBUG_KSNET
        ksnet_printf(
            &kev->ksn_cfg , DEBUG,
            "%sTCP Proxy:%s "
            "Connection closed. Stop listening fd %d\n",
            ANSI_YELLOW, ANSI_NONE, w->fd
        );
        #endif
        
        ksnTCPProxyServerClientDisconnect(tp, w->fd, 1);
    } 
    
    // \todo Process reading error
    else if (received < 0) {  
        
        //        if( errno == EINTR ) {
        //            // OK, just skip it
        //        }
    
        #ifdef DEBUG_KSNET
        ksnet_printf(
            &kev->ksn_cfg, DEBUG,
            "%sTCP Proxy:%s Read error\n", ANSI_YELLOW, ANSI_NONE
        );
        #endif
    }
    
    // Success read. Process package and resend it to UDP proxy when ready
    else {
        
        int rv;
        
        // Parse TCP packet
        rv = ksnTCPProxyPackageProcess(tp, data, data_len);
        
        // Send package to peer by UDP proxy connection
        if(!rv) {
        
            // Address
            const char *addr = (const char *) (tp->buffer + sizeof(ksnTCPProxyMessage_header));
            
            // Port
            int port = ((ksnTCPProxyMessage_header *) tp->buffer)->port;
            
            // Packet        
            const char *packet =  (const char *) (tp->buffer + sizeof(ksnTCPProxyMessage_header) + ((ksnTCPProxyMessage_header *) tp->buffer)->addr_len);
            
            // Packet length
            const size_t packet_len = ((ksnTCPProxyMessage_header *) tp->buffer)->package_len;
            
            // Checksum
            const uint8_t checksum = ((ksnTCPProxyMessage_header *) tp->buffer)->checksum;

            // \todo Send packet to Peer by UDP proxy connection
        } 
        
        // Wrong package
        else if(rv < 0) {
            // \todo Show error message
        }
        
        // Not all package read - continue receiving
        else {
            // Do nothing
        }
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
 * 
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
 * 
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
    ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s Create UDP client/server Proxy at port %d ...\n", 
            ANSI_YELLOW, ANSI_NONE,
            udp_proxy_port);
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
            ev_io_stop (kev->ev_loop, &tpd->w); // Stop TCP Proxy client watcher
            close(tpd->udp_proxy_fd); // Close UDP Proxy connection          
            if(remove_f) 
                pblMapRemove(tp->map, &fd, sizeof(fd), &valueLength); // Remove data from map
        }
        
        // Close TCP Proxy client        
        close(fd); 
        
        // Show disconnect message
        ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sTCP Proxy:%s TCP Proxy client fd %d disconnected\n", 
            ANSI_YELLOW, ANSI_NONE, fd);
}

/**
 * Process TCP proxy package
 * 
 * Read tcp data from socket until end of tcp proxy package, check checksum,
 * take UDP address, port number and UDP package data.
 * 
 * @param tp Pointer to ksnTCPProxyClass
 * @param data Pointer to TCP data received
 * @param data_len TCP data length
 * 
 * @return 0 - done, resend this package; -1 - an error; 1 - reading current package
 */
int ksnTCPProxyPackageProcess(ksnTCPProxyClass *tp, void *data, size_t data_len) {
    
    int rv = 1;
    
    switch(tp->stage) {
        
        // Wait for package began
        case WAIT_FOR_BEGAN:
            tp->ptr = 0;
            tp->stage = WAIT_FOR_END;
            rv = ksnTCPProxyPackageProcess(tp, data, data_len);
            break;
            
        // Wait for package end
        case WAIT_FOR_END: {
            
            // \todo check buffer length
            memcpy(tp->buffer + tp->ptr, data, data_len);  
            tp->ptr += data_len;
            if(tp->ptr >= sizeof(ksnTCPProxyMessage_header)) {
                
                ksnTCPProxyMessage_header *th = (ksnTCPProxyMessage_header *) tp->buffer;
                size_t pkg_len = sizeof(ksnTCPProxyMessage_header) + th->package_len + th->addr_len;
                if(tp->ptr >= pkg_len) {
                    tp->stage = PROCESS_PACKET;
                    rv = ksnTCPProxyPackageProcess(tp, NULL, 0);
                }
            }            
        } 
        break;
            
        // Read process body
        case PROCESS_PACKET: {
            
            ksnTCPProxyMessage_header *th = (ksnTCPProxyMessage_header *) tp->buffer;
            size_t pkg_len = sizeof(ksnTCPProxyMessage_header) + th->package_len + th->addr_len;
            
            // \todo check checksum
            
            if(tp->ptr > pkg_len) {
                tp->ptr = tp->ptr - pkg_len;
                memmove(tp->buffer, tp->buffer + pkg_len, tp->ptr);
                tp->stage = WAIT_FOR_END;
            }
            else tp->stage = WAIT_FOR_BEGAN;
            
            rv = 0;
        }
        break;
            
        // Some wrong stage
        default:
            rv = -1;
            break;
    }
            
    return rv;
}

#undef kev
