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
int ksnL0sStart(ksnL0sClass *kl);
void ksnL0sStop(ksnL0sClass *kl);
void ksnL0sClientDisconnect(ksnL0sClass *kl, int fd, int remove_f);

/**
 * Pointer to ksnetEvMgrClass
 * 
 */
#define kev ((ksnetEvMgrClass*)kl->ke)
#define L0_VERSION 0 ///< L0 Server version

/**
 * Initialize ksnL0s module [class](@ref ksnL0sClass)
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * 
 * @return Pointer to created ksnL0Class or NULL if error occurred
 */
ksnL0sClass *ksnL0sInit(void *ke) {
    
    ksnL0sClass *kl = NULL;
    
    if(((ksnetEvMgrClass*)ke)->ksn_cfg.l0_allow_f) {
        
        kl = malloc(sizeof(ksnL0sClass));
        if(kl != NULL)  {
            kl->ke = ke; // Pointer event manager class
            kl->map = pblMapNewHashMap(); // Create a new hash map            
            ksnL0sStart(kl); // Start L0 Server
        }
    }
    
    return kl;
}

/**
 * Destroy ksnL0s module [class](@ref ksnL0sClass)
 * 
 * @param kl Pointer to ksnL0sClass
 */
void ksnL0sDestroy(ksnL0sClass *kl) {
    
    if(kl != NULL) {
        ksnL0sStop(kl);
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
    ksnL0sClass *kl = w->data; // Pointer to ksnL0sClass
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
        
        ksnL0sClientDisconnect(kl, w->fd, 1);
    } 
    
    // \todo Process reading error
    else if (received < 0) {
        
        //        if( errno == EINTR ) {
        //            // OK, just skip it
        //        }
    
        #ifdef DEBUG_KSNET
        ksnet_printf(
            &kev->ksn_cfg, DEBUG,
            "%sl0 Server:%s "
            "Read error ...%s\n", 
            ANSI_LIGHTCYAN, ANSI_RED, ANSI_NONE
        );
        #endif
    }
    
    // Success read. Process package and resend it to teonet
    else {
                        
        ksnL0sCPacket *packet = (ksnL0sCPacket *)data;
        size_t len, ptr = 0;

        size_t valueLength;
        ksnL0sData* kld = pblMapGet(kl->map, &w->fd, sizeof(w->fd), 
                &valueLength);
            
        // Check received packet
        while(received - ptr >= (len = sizeof(ksnL0sCPacket) + 
                packet->to_length + packet->data_length)) {
            
            if(kld != NULL) {
                    
                // Check initialize packet: 
                // cmd = 0, to_length = 1, data_length = 1 + data_len, 
                // data = client_name
                //
                if(packet->cmd == 0 && packet->to_length == 1 && 
                        !packet->to[0] && packet->data_length) {
                    
                    kld->name = strdup(packet->to + 1);
                    kld->name_length = strlen(kld->name) + 1;
                    
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG, 
                        "%sl0 Server:%s "
                        "Connection initialized, client name: %s ...\n", 
                        ANSI_LIGHTCYAN, ANSI_NONE, kld->name);
                    #endif
                }
                // Resend data to teonet
                else {

                    char out_data[data_len]; // Buffer
                    ksnL0sSPacket *spacket = (ksnL0sSPacket*) out_data;

                    // Create teonet L0 packet
                    spacket->cmd = packet->cmd;
                    spacket->from_length = kld->name_length;
                    memcpy(spacket->from, kld->name, spacket->from_length); 
                    spacket->data_length = packet->data_length;
                    memcpy(spacket->from + spacket->from_length, 
                        packet->to + packet->to_length, spacket->data_length);

                    // Send teonet L0 packet
                    ksnCoreSendCmdto(kev->kc, (char*)packet->to, CMD_L0, 
                            spacket, sizeof(ksnL0sSPacket) + 
                            spacket->from_length + spacket->data_length);
                    
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG, 
                        "%sl0 Server:%s "
                        "Resend command to: %s ...\n", 
                        ANSI_LIGHTCYAN, ANSI_NONE, packet->to);
                    #endif
                }
            }
            
            ptr += len;            
            packet = (void*)packet + len;
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

/**
 * Connect l0 Server client
 * 
 * Called when tcp client connected to TCP l0 Server. Register this client in 
 * map
 * 
 * @param kl Pointer to ksnL0sClass
 * @param fd TCP client connection file descriptor
 * 
 */
void ksnL0sClientConnect(ksnL0sClass *kl, int fd) {
    
    // Set TCP_NODELAY option
    set_tcp_nodelay(kev, fd);

    ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sl0 Server:%s "
            "TCP Proxy client fd %d connected\n", 
            ANSI_LIGHTCYAN, ANSI_NONE, fd);
    
    // Register client in tcp proxy map 
    ksnL0sData data;
    data.name = NULL;
    data.name_length = 0;
    pblMapAdd(kl->map, &fd, sizeof(fd), &data, sizeof(ksnL0sData));
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"

    // Create and start TCP watcher (start client processing)
    size_t valueLength;
    ksnL0sData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &valueLength);
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
 * @param kl Pointer to ksnL0sClass
 * @param fd TCP client connection file descriptor
 * @param remove_f If true than remove disconnected record from map
 * 
 */
void ksnL0sClientDisconnect(ksnL0sClass *kl, int fd, int remove_f) {

    size_t valueLength;

    // Get data from TCP Proxy Clients map, close TCP watcher and remove this 
    // data record from map
    ksnL0sData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &valueLength); 
    if(kld != NULL) {

        // Stop TCP Proxy client watcher
        ev_io_stop(kev->ev_loop, &kld->w);
        close(fd); 

        // Show disconnect message
        ksnet_printf(&kev->ksn_cfg, CONNECT, 
            "%sl0 Server:%s "
            "TCP Proxy client fd %d disconnected\n", 
            ANSI_LIGHTCYAN, ANSI_NONE, fd);
        
        // Free name
        if(kld->name != NULL) {
            
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
    
    ksnL0sClientConnect(w->data, fd);    
}

/**
 * Start l0 Server
 * 
 * Create and start l0 Server
 * 
 * @param kl Pointer to ksnL0sClass
 * 
 * @return If return value > 0 than server was created successfully
 */
int ksnL0sStart(ksnL0sClass *kl) {
    
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
 * @param kl Pointer to ksnL0sClass
 * 
 */
void ksnL0sStop(ksnL0sClass *kl) {
    
    // If server started
    if(kev->ksn_cfg.l0_allow_f && kl->fd) {
        
        // Disconnect all clients
        PblIterator *it = pblMapIteratorReverseNew(kl->map);
        if(it != NULL) {
            while(pblIteratorHasPrevious(it) > 0) {
                void *entry = pblIteratorPrevious(it);
                int *fd = (int *) pblMapEntryKey(entry);
                ksnL0sClientDisconnect(kl, *fd, 0);
            }
            pblIteratorFree(it);
        }
                
        // Clear map
        pblMapClear(kl->map);
        
        // Stop the server
        ksnTcpServerStop(kev->kt, kl->fd);
    }
}

/**
 * Process CMD_L0 teonet command
 *
 * @param kl
 * @param rd
 * @return
 */
int cmd_l0_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd) {
    
    int retval = 1;
    ksnL0sSPacket *data = rd->data;
    
    // Execute L0 client command
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG, 
        "%sl0 Server:%s "
        "Got command No %d from %s client with %d bytes data\n", 
        ANSI_LIGHTCYAN, ANSI_NONE, data->cmd, data->from, data->data_length);
    #endif

    // \todo Process command
//    int ksnCommandCheck(ksnCommandClass *kco, ksnCorePacketData *rd);   
//    retval = ksnCommandCheck(ke->kc->kco, ksnCorePacketData *rd)
    
    return retval;
}

#undef kev
