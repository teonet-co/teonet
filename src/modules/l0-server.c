/** 
 * File:   l0-server.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * L0 Server module
 *
 * Created on October 8, 2015, 1:38 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ev_mgr.h"
#include "l0-server.h"
#include "utils/rlutil.h"

// Local functions
int ksnLNullStart(ksnLNullClass *kl);
void ksnLNullStop(ksnLNullClass *kl);
void ksnLNullClientDisconnect(ksnLNullClass *kl, int fd, int remove_f);
ksnet_arp_data *ksnLNullSendFromL0(ksnLNullClass *kl, ksnLNullCPacket *packet, 
        char *cname, size_t cname_length);

/**
 * Pointer to ksnetEvMgrClass
 * 
 */
#define kev ((ksnetEvMgrClass*)kl->ke)
#define L0_VERSION 0 ///< L0 Server version

/**
 * Initialize ksnLNull module [class](@ref ksnLNullClass)
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * 
 * @return Pointer to created ksnLNullClass or NULL if error occurred
 */
ksnLNullClass *ksnLNullInit(void *ke) {
    
    ksnLNullClass *kl = NULL;
    
    if(((ksnetEvMgrClass*)ke)->ksn_cfg.l0_allow_f) {
        
        kl = malloc(sizeof(ksnLNullClass));
        if(kl != NULL)  {
            kl->ke = ke; // Pointer event manager class
            kl->map = pblMapNewHashMap(); // Create a new hash map      
            kl->map_n = pblMapNewHashMap(); // Create a new hash map     
            ksnLNullStart(kl); // Start L0 Server
        }
    }
    
    return kl;
}

/**
 * Destroy ksnLNull module [class](@ref ksnLNullClass)
 * 
 * @param kl Pointer to ksnLNullClass
 */
void ksnLNullDestroy(ksnLNullClass *kl) {
    
    if(kl != NULL) {
        ksnLNullStop(kl);
        free(kl->map_n);
        free(kl->map);
        free(kl);
    }
}

/**
 * L0 Server client callback
 * 
 * Get packet from L0 Server client connection and resend it to teonet
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * 
 */
void cmd_l0_read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
    
    size_t data_len = KSN_BUFFER_DB_SIZE; // Buffer length
    ksnLNullClass *kl = w->data; // Pointer to ksnLNullClass
    char data[data_len]; // Buffer
    
    // Read TCP data
    ssize_t received = read(w->fd, data, data_len);
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG, 
            "%sl0 Server:%s "
            "Got something from fd %d w->events = %d, received = %d ...\n", 
            ANSI_LIGHTCYAN, ANSI_NONE, w->fd, w->events, (int)received);
    #endif

    // Disconnect client:
    // Close TCP connections and remove data from l0 clients map
    if(!received) {        
        
        #ifdef DEBUG_KSNET
        ksnet_printf(
            &kev->ksn_cfg , DEBUG,
            "%sl0 Server:%s "
            "Connection closed. Stop listening fd %d ...\n",
            ANSI_LIGHTCYAN, ANSI_NONE, w->fd
        );
        #endif
        
        ksnLNullClientDisconnect(kl, w->fd, 1);
    } 
    
    // \todo Process reading error
    else if (received < 0) {
        
        //        if( errno == EINTR ) {
        //            // OK, just skip it
        //        }
    
//        #ifdef DEBUG_KSNET
//        ksnet_printf(
//            &kev->ksn_cfg, DEBUG,
//            "%sl0 Server:%s "
//            "Read error ...%s\n", 
//            ANSI_LIGHTCYAN, ANSI_RED, ANSI_NONE
//        );
//        #endif
    }
    
    // Success read. Process package and resend it to teonet
    else {
                        
        ksnLNullCPacket *packet = (ksnLNullCPacket *)data;
        size_t len, ptr = 0;

        size_t valueLength;
        ksnLNullData* kld = pblMapGet(kl->map, &w->fd, sizeof(w->fd), 
                &valueLength);
            
        if(kld != NULL) {
                    
            // Check received packet
            while(received - ptr >= (len = sizeof(ksnLNullCPacket) + 
                    packet->peer_name_length + packet->data_length)) {

                    // Check initialize packet: 
                    // cmd = 0, to_length = 1, data_length = 1 + data_len, 
                    // data = client_name
                    //
                    if(packet->cmd == 0 && packet->peer_name_length == 1 && 
                            !packet->peer_name[0] && packet->data_length) {

                        kld->name = strdup(packet->peer_name + 1);
                        kld->name_length = strlen(kld->name) + 1;
                        
                        pblMapAdd(kl->map_n, kld->name, kld->name_length, 
                                &w->fd, sizeof(w->fd));
                        
                        #ifdef DEBUG_KSNET
                        ksnet_printf(&kev->ksn_cfg, DEBUG, 
                            "%sl0 Server:%s "
                            "Connection initialized, client name: %s ...\n", 
                            ANSI_LIGHTCYAN, ANSI_NONE, kld->name);
                        #endif
                    }
                    // Resend data to teonet
                    else {

                        ksnLNullSendFromL0(kl, packet, kld->name, kld->name_length);
                    }

                ptr += len;            
                packet = (void*)packet + len;
            } 
        }
        
        if(received - ptr > 0) {
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&kev->ksn_cfg, DEBUG, 
                "%sl0 Server:%s "
                "Wrong package, %d bytes ...%s\n", 
                ANSI_LIGHTCYAN, ANSI_RED, received - ptr, ANSI_NONE);
            #endif
        }
    }
}

//Pointer to ksnet_arp_data or NULL if to is absent

/**
 * Send data received from L0 client to teonet peer
 * 
 * @param kl
 * @param packet L0 packet received from L0 client
 * @param cname L0 client name (include trailing zero)
 * @param cname_length Length of the L0 client name
 * @return Pointer to ksnet_arp_data or NULL if to peer is absent
 */
ksnet_arp_data *ksnLNullSendFromL0(ksnLNullClass *kl, ksnLNullCPacket *packet, 
        char *cname, size_t cname_length) {
    
    size_t out_data_len = sizeof(ksnLNullSPacket) + cname_length + packet->data_length;            
    //char out_data[data_len]; // Buffer
    char *out_data = malloc(out_data_len);
    memset(out_data, 0, out_data_len);
    ksnLNullSPacket *spacket = (ksnLNullSPacket*) out_data;

    // Create teonet L0 packet
    spacket->cmd = packet->cmd;
    spacket->from_length = cname_length;
    memcpy(spacket->from, cname, cname_length); 
    spacket->data_length = packet->data_length;
    memcpy(spacket->from + spacket->from_length, 
        packet->peer_name + packet->peer_name_length, spacket->data_length);

    // Send teonet L0 packet
    ksnet_arp_data *arp = ksnCoreSendCmdto(kev->kc, (char*)packet->peer_name, CMD_L0, 
            spacket, out_data_len);

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG, 
        "%sl0 Server:%s "
        "Send command from L0 %s client to %s peer ...\n", 
        ANSI_LIGHTCYAN, ANSI_NONE, spacket->from, packet->peer_name);
    #endif

    free(out_data);

    return arp;
}

/**
 * Send data to L0 client. Usually it is an answer to request from L0 client
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @param addr IP address of remote peer
 * @param port Port of remote peer
 * @param cname L0 client name (include trailing zero)
 * @param cname_length Length of the L0 client name
 * @param cmd Command
 * @param data Data
 * @param data_len Data length
 * 
 * @return Return 0 if success; -1 if data length is too lage (more than 32319)
 */
int ksnLNullSendToL0(void *ke, char *addr, int port, char *cname, 
        size_t cname_length, uint8_t cmd, void *data, size_t data_len) {
    
    // Create L0 packet
    size_t out_data_len = sizeof(ksnLNullSPacket) + cname_length + data_len;    
    //char out_data[out_data_len]; // Buffer
    char *out_data = malloc(out_data_len);
    memset(out_data, 0, out_data_len);
    ksnLNullSPacket *spacket = (ksnLNullSPacket*)out_data;
    
    // Create teonet L0 packet
    spacket->cmd = cmd;
    spacket->from_length = cname_length;
    memcpy(spacket->from, cname, cname_length);
    spacket->data_length = data_len;
    memcpy(spacket->from + spacket->from_length, data, data_len);
    
    int rv = ksnCoreSendto(((ksnetEvMgrClass*)ke)->kc, addr, port, CMD_L0TO, 
            out_data, out_data_len);    
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&((ksnetEvMgrClass*)ke)->ksn_cfg, DEBUG, 
        "%sl0 Server:%s "
        "Send command to L0 client %s ...\n", 
        ANSI_LIGHTCYAN, ANSI_NONE, spacket->from);
    #endif

    free(out_data);
    
    return rv;
}

/**
 * Connect l0 Server client
 * 
 * Called when tcp client connected to TCP l0 Server. Register this client in 
 * map
 * 
 * @param kl Pointer to ksnLNullClass
 * @param fd TCP client connection file descriptor
 * 
 */
void ksnLNullClientConnect(ksnLNullClass *kl, int fd) {
    
    // Set TCP_NODELAY option
    set_tcp_nodelay(kev, fd);

    ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sl0 Server:%s "
            "L0 client with fd %d connected\n", 
            ANSI_LIGHTCYAN, ANSI_NONE, fd);
    
    // Register client in tcp proxy map 
    ksnLNullData data;
    data.name = NULL;
    data.name_length = 0;
    pblMapAdd(kl->map, &fd, sizeof(fd), &data, sizeof(ksnLNullData));
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"

    // Create and start TCP watcher (start client processing)
    size_t valueLength;
    ksnLNullData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &valueLength);
    if(kld != NULL) {   
        
        // Create and start TCP watcher (start TCP client processing)
        ev_init (&kld->w, cmd_l0_read_cb);
        ev_io_set (&kld->w, fd, EV_READ);
        kld->w.data = kl;
        ev_io_start (kev->ev_loop, &kld->w);        
    }
    
    // Error: can't register TCP fd in tcp proxy map
    else {
        // \todo process error: can't register TCP fd in tcp proxy map
    }

    #pragma GCC diagnostic pop
}

/**
 * Disconnect l0 Server client
 * 
 * Called when client disconnected or when the L0 Server closing. Close TCP 
 * connections and remove data from L0 Server clients map.
 * 
 * @param kl Pointer to ksnLNullClass
 * @param fd TCP client connection file descriptor
 * @param remove_f If true than remove disconnected record from map
 * 
 */
void ksnLNullClientDisconnect(ksnLNullClass *kl, int fd, int remove_f) {

    size_t valueLength;

    // Get data from L0 Clients map, close TCP watcher and remove this 
    // data record from map
    ksnLNullData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &valueLength); 
    if(kld != NULL) {

        // Stop L0 client watcher
        ev_io_stop(kev->ev_loop, &kld->w);
        close(fd); 

        // Show disconnect message
        ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sl0 Server:%s "
            "L0 client with fd %d disconnected\n", 
            ANSI_LIGHTCYAN, ANSI_NONE, fd);
        
        // Free name
        if(kld->name != NULL) {
            
            if(remove_f) 
                pblMapRemove(kl->map_n, kld->name, kld->name_length, 
                        &valueLength);
            
            free(kld->name);
            kld->name = NULL;
            kld->name_length = 0;
        }

        // Remove data from map
        if(remove_f) 
            pblMapRemove(kl->map, &fd, sizeof(fd), &valueLength);
    }        
}

/**
 * L0 Server accept callback
 * 
 * Register client, create event watchers with clients callback
 * 
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * @param fd File description of created client connection
 * 
 */
void cmd_l0_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *w,
                       int revents, int fd) {
    
    ksnLNullClientConnect(w->data, fd);    
}

/**
 * Start l0 Server
 * 
 * Create and start l0 Server
 * 
 * @param kl Pointer to ksnLNullClass
 * 
 * @return If return value > 0 than server was created successfully
 */
int ksnLNullStart(ksnLNullClass *kl) {
    
    int fd = 0;
    
    if(kev->ksn_cfg.l0_allow_f) {
        
        // Create l0 server at port, which will wait client connections
        int port_created;
        if((fd = ksnTcpServerCreate(
                    kev->kt, 
                    kev->ksn_cfg.l0_tcp_port,
                    cmd_l0_accept_cb, 
                    kl, 
                    &port_created)) > 0) {

            ksnet_printf(&kev->ksn_cfg, MESSAGE, 
                    "%sl0 Server:%s "
                    "l0 Server fd %d started at port %d\n", 
                    ANSI_LIGHTCYAN, ANSI_NONE,
                    fd, port_created);

            kev->ksn_cfg.tcp_port = port_created;
            kl->fd = fd;
        }
    }
    
    return fd;
}

/**
 * Stop L0 Server
 * 
 * Disconnect all connected clients and stop l0 server
 * 
 * @param kl Pointer to ksnLNullClass
 * 
 */
void ksnLNullStop(ksnLNullClass *kl) {
    
    // If server started
    if(kev->ksn_cfg.l0_allow_f && kl->fd) {
        
        // Disconnect all clients
        PblIterator *it = pblMapIteratorReverseNew(kl->map);
        if(it != NULL) {
            while(pblIteratorHasPrevious(it) > 0) {
                void *entry = pblIteratorPrevious(it);
                int *fd = (int *) pblMapEntryKey(entry);
                ksnLNullClientDisconnect(kl, *fd, 0);
            }
            pblIteratorFree(it);
        }
                
        // Clear maps
        pblMapClear(kl->map_n);
        pblMapClear(kl->map);
        
        // Stop the server
        ksnTcpServerStop(kev->kt, kl->fd);
    }
}

/**
 * Process CMD_L0 teonet command
 *
 * @param ke
 * @param rd
 * @return
 */
int cmd_l0_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd) {
    
    int retval = 1;
    ksnLNullSPacket *data = rd->data;
        
    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG, 
        "%sl0 Server:%s "
        "Got command No %d from %s client with %d bytes data\n", 
        ANSI_LIGHTCYAN, ANSI_NONE, data->cmd, data->from, data->data_length);
    #endif

    // Process command
    rd->cmd = data->cmd;
    rd->from = data->from;
    rd->from_len = data->from_length;
    rd->data = data->from + data->from_length;
    rd->data_len = data->data_length;
    rd->l0_f = 1;
    
    // Execute L0 client command
    retval = ksnCommandCheck(ke->kc->kco, rd);
    
    //printf("ksnCommandCheck => %s, command No %d, data: %s\n", (retval ? "processed" : "not processed"), rd->cmd, (char*)rd->data);
    
    return retval;
}

/**
 * Process CMD_L0TO teonet command
 *
 * @param ke
 * @param rd
 * @return
 */
int cmd_l0to_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd) {
    
    int retval = 1;
    ksnLNullSPacket *data = rd->data;
        
    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG, 
        "%sl0 Server:%s "
        "Got command No %d to \"%s\" L0 client from peer \"%s\" with %d bytes data\n", 
        ANSI_LIGHTCYAN, ANSI_NONE, 
        data->cmd, data->from, rd->from, data->data_length);
    #endif
    
    // Create L0 packet
    size_t out_data_len = sizeof(ksnLNullCPacket) + rd->from_len + data->data_length;
    char *out_data = malloc(out_data_len);
    memset(out_data, 0, out_data_len);
    ksnLNullCPacket *packet = (ksnLNullCPacket*)out_data;
    packet->cmd = data->cmd;
    packet->peer_name_length = rd->from_len;
    packet->data_length = data->data_length;
    memcpy(packet->peer_name, rd->from, rd->from_len);
    memcpy(packet->peer_name + rd->from_len, data->from + data->from_length, 
            data->data_length);
    
    // Send command to L0 client
    int *fd;
    size_t snd;
    size_t valueLength;
    fd = pblMapGet(ke->kl->map_n, data->from, data->from_length, &valueLength);
    if((snd = write(*fd, out_data, out_data_len)) >= 0);
    
    #ifdef DEBUG_KSNET
    void *packet_data = packet->peer_name + packet->peer_name_length;
    ksnet_printf(&ke->ksn_cfg, DEBUG, 
        "%sl0 Server:%s "
        "Send %d bytes to \"%s\" L0 client: %d bytes data, from peer \"%s\": %s\n", 
        ANSI_LIGHTCYAN, ANSI_NONE, 
        (int)snd, data->from, 
        packet->data_length, packet->peer_name, 
        packet_data);
    #endif

    free(out_data);
    
    return retval;
}

#undef kev
