/** 
 * \file   net_recon.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet reconnect module.
 * 
 * Reconnect to disconnected peer
 * 
 * CMD_RECONNECT
 * 
 * When disconnect from peer (not r-host peer):
 * 
 * - Story disconnected host in reconnect table
 * - Ask r-host to reconnect (send reconnect command)
 * - Wait for connected to peer or wait signal from r-host (peer does not exists)
 *   - Resend reconnect command if not connected and r-host signal has not received during timeout
 *   - Remove "disconnected host" from "reconnect table" and exit
 *
 * Created on November 5, 2015, 11:19 AM
 */

#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "net_recon.h"
#include "utils/rlutil.h"

#define MODULE _ANSI_GREEN "net_reconnect" _ANSI_NONE

/**
 * Reconnect command map data structure
 */
typedef struct reconnect_map_data {
    
    ksnReconnectClass *kr;
    ksnCQueData *cq;
    char *peer;
    
} reconnect_map_data;

// Local functions
static void ksnReconnectDestroy(ksnReconnectClass *this);
static int ksnReconnectSend(ksnReconnectClass *this, const char *peer);
static int ksnReconnectSendAnswer(ksnReconnectClass *this, const char *peer, 
        const char* peer_to_reconnect);
static int ksnReconnectProcess(ksnReconnectClass *this, ksnCorePacketData *rd);
static int ksnReconnectProcessAnswer(ksnReconnectClass *this, ksnCorePacketData *rd);
static void ksnReconnectCQueCallback(uint32_t id, int type, void *data);
//
int send_cmd_connected_cb(ksnetArpClass *ka, char *name, ksnet_arp_data *arp_data, void *data);

#define kev ((ksnetEvMgrClass*)kcor->ke)
    
/**
 * CallbackQueue callback
 * 
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback 
 * @param data User data selected in ksnCQueAdd function
 *  
 */
static void ksnReconnectCQueCallback(uint32_t id, int type, void *data) {
    
    #define kcor ((ksnCoreClass*)((ksnCommandClass*) \
                    map_data->kr->kco)->kc)
    #define karp kcor->ka

    reconnect_map_data *map_data = (reconnect_map_data *) data;
    size_t valueLen, peer_len = strlen(map_data->peer) + 1;
    map_data = pblMapRemove(map_data->kr->map, (void*)map_data->peer, 
            peer_len, &valueLen);
    
    // If map record successfully remover
    if(map_data != NULL && map_data != (void*)-1) {
    
        // Timeout callback
        if(type == 0) {

            // Check peer is connected and resend CMD_RECONNECT if not
            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG_VV,
                    "callback: Got timeout for peer \"%s\"\n",
                    map_data->peer);
            #endif
            
            // If connected
            //ksnet_arp_data *arp;
            if((/*arp =*/ ksnetArpGet(karp, map_data->peer)) != NULL) {
                
                // ... do nothing ...
                #ifdef DEBUG_KSNET
                ksn_puts(kev, MODULE, DEBUG_VV,
                    "callback: Has already connected - stop");
                #endif
            }
            
            // If not connected send Reconnect command
            else { 
                
                #ifdef DEBUG_KSNET
                ksn_puts(kev, MODULE, DEBUG_VV,
                    "callback: Has not connected yet - reconnect");
                #endif

                map_data->kr->send(map_data->kr, map_data->peer);
            }
        }

        // Got the CMD_RECONNECT_ANSWER from r-host, the peer has not connected 
        // to r-host, stop connecting
        else {
            // ... do nothing ...
            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG_VV,
                "callback: Got the CMD_RECONNECT_ANSWER for peer \"%s\" - stop\n",
                map_data->peer);
            #endif
        }
        
        // Free map data
        free(map_data->peer);
        free(map_data);
        
    }    
    
    #undef karp
    #undef kcor
}

#define kcor ((ksnCoreClass*)(((ksnCommandClass*)this->kco)->kc))

/**
 * Send CMD_RECONNECT command to r-host
 * 
 * @param this Pointer to ksnReconnectClass
 * @param peer Peer name to reconnect
 * @return Zero if success or error
 * @retval  0 Send command successfully
 * @retval  1 The reconnect request with selected peer already in progress
 * @retval -1 Has not connected to r-host
 * @retval -2 PBL error
 */
static int ksnReconnectSend(ksnReconnectClass *this, const char *peer) {
    
    int retval = 0;
    
    char *r_host = kev->ksn_cfg.r_host_name;
    ksnet_arp_data *arp_data;

    // If connected to r-host
    if(r_host[0] && (arp_data = (ksnet_arp_data *)ksnetArpGet(kcor->ka, r_host)) != NULL) {

        int rc = 0;
        reconnect_map_data md;
        size_t valueLen, peer_len = strlen(peer) + 1;
        
        memset(&md, 0, sizeof(md));
        md.kr = this;

        // Check exists and add new reconnect request
        if(pblMapGet(this->map, (void*)peer, peer_len, &valueLen) == NULL && 
           (rc = pblMapAdd(this->map, (void*)peer, peer_len, &md, 
                sizeof(md))) >= 0) {

            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG_VV, 
                    "send reconnect request with peer \"%s\" "
                    "(and save it to queue)\n",                     
                    peer);
            #endif
            
            // Send command reconnect to r-host
            arp_data = ksnCoreSendCmdto(((ksnCommandClass*)this->kco)->kc, r_host, 
                    CMD_RECONNECT, (void*)peer, peer_len);

            // Has not connected to r-host
            if(arp_data == NULL) {
                
                retval = -1; 
                size_t vl;
                pblMapRemoveFree(this->map, (void*)peer, peer_len, &vl);
            }

            // Add to queue and map if Send successfully
            else {

                // Add to class map and set it as ksnCQueAdd data
                reconnect_map_data *map_data;
                if((map_data = pblMapGet(this->map, (void*)peer, peer_len, 
                        &valueLen)) != NULL) {

                    // Add peer name to map data
                    map_data->peer = strdup(peer);
                    
                    // Add callback to queue and wait timeout after ~5 sec ...
                    map_data->cq = ksnCQueAdd(((ksnetEvMgrClass*)kcor->ke)->kq, 
                        this->callback, 5.0/*CHECK_EVENTS_AFTER / 2.0*/, map_data);
                }
                
                // PBL error
                else retval = -2;
            }
        }
        
        // This request already exist
        else if(rc == 0) {
            
            #ifdef DEBUG_KSNET
            ksn_puts(kev, MODULE, DEBUG_VV,
                "send skipped - reconnect request already running");
            #endif
            
            retval = 1;
        }
        
        // PBL error
        else {
            
            #ifdef DEBUG_KSNET
            ksn_puts(kev, MODULE, DEBUG_VV, "send skipped - PBL error");
            #endif

            retval = -2;        
        }
    }

    // Has not connected to r-host
    else retval = -1;
        
    return retval;
}

/**
 * Send CMD_RECONNECT_ANSWER command to r-host
 * 
 * @param this Pointer to ksnReconnectClass
 * @param peer Peer name to send
 * @param peer_to_reconnect Peer name to reconnect
 * @return Zero if success or error
 * @retval  0 Send command successfully
 * @retval -1 Has not connected to peer
 */
static int ksnReconnectSendAnswer(ksnReconnectClass *this, const char *peer, 
        const char* peer_to_reconnect) {
    
    #ifdef DEBUG_KSNET
    ksn_printf(kev, MODULE, DEBUG_VV, "send answer to \"%s\"\n", peer);
    #endif
    
    // Send command reconnect answer to peer
    ksnet_arp_data *arp = ksnCoreSendCmdto(((ksnCommandClass*)this->kco)->kc, 
        (char*)peer, CMD_RECONNECT_ANSWER, 
        (void*)peer_to_reconnect, strlen(peer_to_reconnect) + 1);
            
    return arp == NULL ? -1 : 0;
}

/**
 * Process CMD_RECONNECT command 
 * 
 * Got by r-host from peer wanted reconnect with peer rd->data
 * 
 * @param this Pointer to ksnReconnectClass
 * @param rd Pointer to ksnCorePacketData
 * @return 
 */
static int ksnReconnectProcess(ksnReconnectClass *this, ksnCorePacketData *rd) {
    
    #define kcom ((ksnCommandClass*)this->kco)
    #define karp ((ksnCoreClass*)kcom->kc)->ka

    #ifdef DEBUG_KSNET
    ksn_printf(kev, MODULE, DEBUG_VV,
        "process reconnect command from peer \"%s\" with \"%s\"\n", 
        rd->from, rd->data
    );
    #endif
    
    ksnet_arp_data *arp_data;
            
    // Send reconnect answer command if requested peer is disconnected    
    if((arp_data = (ksnet_arp_data *)ksnetArpGet(karp, rd->data)) == NULL)  
        
            this->sendAnswer(this, rd->from, rd->data);

    // Reconnect sender (rd->from) with requested peer (rd->data)
    else {
        
//        // Send connect command request to peer
//        ksnCommandSendCmdConnect(kcom, rd->data, rd->from, rd->addr, rd->port);
//        
//        // Send connect command request to sender
//        ksnCommandSendCmdConnect(kcom, rd->from, rd->data, arp->addr, arp->port);   
        
        // Send connect command request to peers
        send_cmd_connected_cb(karp, rd->data, arp_data, rd);
    }
    
    return 1;
    
    #undef karp
    #undef kcom
}

/**
 * Process CMD_RECONNECT_ANSWER command 
 * 
 * Got from r-host if peer to reconnect has not connected to r-host
 * 
 * @param this Pointer to ksnReconnectClass
 * @param rd Pointer to ksnCorePacketData
 * @return 
 */
static int ksnReconnectProcessAnswer(ksnReconnectClass *this, ksnCorePacketData *rd) {
    
    #ifdef DEBUG_KSNET
    ksn_printf(kev, MODULE, DEBUG_VV,
        "process reconnect answer command with peer \"%s\"\n", 
        rd->data);
    #endif
    
    int retval = 1;
    
    // Get record from map    
    // rd->data; // Peer to reconnect
    size_t valueLen;     
    reconnect_map_data *map_data = NULL;
    if((map_data = pblMapGet(this->map, (void*)rd->data, strlen(rd->data) + 1, 
            &valueLen)) != NULL) {
    
        // Execute callback queue (with type success)
        //map_data->peer = rd->data;
        ksnCQueExec(((ksnetEvMgrClass*)
            ((ksnCoreClass*)((ksnCommandClass*)this->kco)->kc)->ke)->kq, 
                map_data->cq->id /*success_id*/);

    }
    
    return retval;
}

/**
 * Initialize ksnReconnectClass
 * 
 * @param kco Pointer to ksnCommandClass
 * @return 
 */
ksnReconnectClass *ksnReconnectInit(void *kco) {
    
    #define kev_ ((ksnetEvMgrClass*)((ksnCoreClass*) \
                        (((ksnCommandClass*)kco)->kc))->ke)
          
    #ifdef DEBUG_KSNET
    ksn_puts(kev_, MODULE, DEBUG_VV, "Initialize ksnReconnectClass");
    #endif

    #undef kev_
    
    ksnReconnectClass *this = malloc(sizeof(ksnReconnectClass));
    
    this->map = pblMapNewHashMap();
    this->this = this;
    this->kco = kco;
    
    this->send = ksnReconnectSend;
    this->sendAnswer = ksnReconnectSendAnswer;
    this->process = ksnReconnectProcess;
    this->processAnswer = ksnReconnectProcessAnswer;
    this->callback = ksnReconnectCQueCallback;
    this->destroy = ksnReconnectDestroy;
    
    return this;
}

/**
 * Destroy ksnReconnectClass
 * 
 * @param this Pointer to ksnReconnectClass
 */
static void ksnReconnectDestroy(ksnReconnectClass *this) {
    
    if(this != NULL) {
        
        #ifdef DEBUG_KSNET
        ksn_puts(kev, MODULE, DEBUG_VV, "Destroy ksnReconnectClass");
        #endif
    
        pblMapFree(this->map);
        free(this);
    }
}
