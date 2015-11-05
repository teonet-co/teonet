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

/**
 * Reconnect command map data structure
 */
typedef struct reconnect_map_data {
    
    ksnReconnectClass *kr;
    ksnCQueData *cq;
    char *peer;
    
} reconnect_map_data;

// Local functions
void ksnReconnectDestroy(ksnReconnectClass *this);
int ksnReconnectSend(ksnReconnectClass *this, const char *peer);
int ksnReconnectSendAnswer(ksnReconnectClass *this, const char *peer, 
        const char* peer_to_reconnect);
int ksnReconnectProcess(ksnReconnectClass *this, ksnCorePacketData *rd);
int ksnReconnectProcessAnswer(ksnReconnectClass *this, ksnCorePacketData *rd);
void ksnReconnectCQueCallback(uint32_t id, int type, void *data);

// \todo insert call to send CMD_RECONNECT somewhere in teonet code

/**
 * CallbackQueue callback
 * 
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback 
 * @param data User data selected in ksnCQueAdd function
 *  
 */
void ksnReconnectCQueCallback(uint32_t id, int type, void *data) {
    
    reconnect_map_data *map_data = (reconnect_map_data *) data;
    size_t valueLen, peer_len = strlen(map_data->peer) + 1;
    map_data = pblMapRemove(map_data->kr->map, (void*)map_data->peer, 
            peer_len, &valueLen);

    // If map record successfully remover
    if(map_data != NULL && map_data != (void*)-1) {
    
        // Timeout callback
        if(type == 0) {

            // Check peer is connected and resend CMD_RECONNECT if not
            
            // If connected
            ksnet_arp_data *arp;
            if((arp = ksnetArpGet(((ksnCoreClass*)((ksnCommandClass*)
                ((ksnReconnectClass*)map_data->kr)->kco)->kc)->ka, 
                    map_data->peer)) != NULL) {
                
                // ... do nothing ...
            }
            
            // If not connected send Reconnect command
            else ((ksnReconnectClass*)map_data->kr)->send(
                        ((ksnReconnectClass*)map_data->kr), map_data->peer);
        }

        // Got the CMD_RECONNECT_ANSWER from r-host, the peer has not connected 
        // to r-host, stop connecting
        else {
            // ... do nothing ...
        }
        
        // Free map data
        free(map_data->peer);
        free(map_data);
    }    
}

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
int ksnReconnectSend(ksnReconnectClass *this, const char *peer) {
    
    int retval = 0;
    
    ksnCoreClass *kc = ((ksnCoreClass*)(((ksnCommandClass*)this->kco)->kc));
    ksnet_cfg *conf = &((ksnetEvMgrClass*)kc->ke)->ksn_cfg;
    char *r_host = conf->r_host_name;
    ksnet_arp_data *arp;

    // If connected to r-host
    if(r_host[0] && (arp = ksnetArpGet(kc->ka, r_host)) != NULL) {

        int rc;
        reconnect_map_data md;
        memset(&md, 0, sizeof(md));
        size_t peer_len = strlen(peer) + 1;
        md.kr = this;

        if((rc = pblMapAdd(this->map, (void*)peer, peer_len, &md, sizeof(md))) 
                >= 0) {

            // Send command reconnect to r-host
            arp = ksnCoreSendCmdto(((ksnCommandClass*)this->kco)->kc, r_host, 
                    CMD_RECONNECT, (void*)peer, peer_len);

            // Has not connected to r-host
            if(arp == NULL) {
                
                retval = -1; 
                size_t vl;
                void *rm = pblMapRemove(this->map, (void*)peer, peer_len, &vl);
                if(rm != NULL && rm != (void*)-1) free(rm);
            }

            // Add to queue and map if Send successfully
            else {

                // Add to class map and set it as ksnCQueAdd data
                size_t valueLen;     
                reconnect_map_data *map_data = NULL;
                if((map_data = pblMapGet(this->map, (void*)peer, peer_len, 
                        &valueLen)) != NULL) {

                    // Add peer name to map data
                    map_data->peer = strdup(peer);
                    
                    // Add callback to queue and wait timeout after ~5 sec ...
                    map_data->cq = ksnCQueAdd(((ksnetEvMgrClass*)kc->ke)->kq, 
                            this->callback, CHECK_EVENTS_AFTER / 2, map_data);
                }
                
                // PBL error
                else retval = -2;
            }
        }
        
        // This request already exist
        else if(rc == 0) retval = 1;
        
        // PBL error
        else retval = -2;        
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
int ksnReconnectSendAnswer(ksnReconnectClass *this, const char *peer, 
        const char* peer_to_reconnect) {
    
    // Send command reconnect answer to peer
    ksnet_arp_data *arp = ksnCoreSendCmdto(((ksnCommandClass*)this->kco)->kc, 
        (char*)peer, CMD_RECONNECT, 
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
int ksnReconnectProcess(ksnReconnectClass *this, ksnCorePacketData *rd) {
    
    // Send reconnect answer command if requested peer is disconnected    
    if(ksnetArpGet(((ksnCoreClass*)((ksnCommandClass*)this->kco)->kc)->ka, 
        rd->data) == NULL)  
        
            this->sendAnswer(this, rd->from, rd->data);

    // \todo Reconnect sender (rd->from) with requested peer (rd->data)
    else {
        
    }
    
    return 1;
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
int ksnReconnectProcessAnswer(ksnReconnectClass *this, ksnCorePacketData *rd) {
    
    int retval = 1;
    
    // Get record from map    
    // rd->data; // Peer to reconnect
    size_t valueLen;     
    reconnect_map_data *map_data = NULL;
    if((map_data = pblMapGet(this->map, (void*)rd->data, strlen(rd->data) + 1, 
            &valueLen)) != NULL) {
    
        // Execute callback queue (with type success)
        map_data->peer = rd->data;
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
void ksnReconnectDestroy(ksnReconnectClass *this) {
    
    if(this != NULL) {
        
        pblMapFree(this->map);
        free(this);
    }
}
