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

#include "ev_mgr.h"
#include "l0-server.h"
#include "utils/rlutil.h"

// Local functions
int ksnL0sStart(ksnL0sClass *kl);

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
            kl->arp = pblMapNewHashMap(); // Create a new hash map            
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
        free(kl->arp);
        free(kl);
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
    
    // ksnTCPProxyServerClientConnect(w->data, fd);    
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
                    "%sl0 Server:%s l0 Server fd %d started at port %d\n", 
                    ANSI_LIGHTCYAN, ANSI_NONE,
                    fd, port_created);

            kev->ksn_cfg.tcp_port = port_created;
            kl->fd = fd;
        }
    }
    
    return fd;
}

#undef kev
