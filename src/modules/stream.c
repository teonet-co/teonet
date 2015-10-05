/** 
 * File:   stream.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet stream module
 *
 * Created on October 3, 2015, 3:01 PM
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "stream.h"
#include "ev_mgr.h"
#include "utils/rlutil.h"

#define kev ((ksnetEvMgrClass*)ks->ke)

/**
 * Initialize Stream module
 * 
 * @param ke Pointer to 
 * @return 
 */
ksnStreamClass *ksnStreamInit(void *ke) {
    
    ksnStreamClass *ks = malloc(sizeof(ksnStreamClass));
    ks->map = pblMapNewHashMap();
    ks->ke = ke;
    
    return ks;
}

/**
 * Stream module destroy
 * 
 * @param ks Pointer to ksnStreamClass
 */
void ksnStreamDestroy(ksnStreamClass *ks) {
    
    if(ks != NULL) {
        pblMapFree(ks->map);
        free(ks);    
    }
}

/**
 * Send stream command to peer
 * 
 * @param ks
 * @param to_peer
 * @param stream_name
 * @param cmd
 * @param data
 * @param data_len
 * 
 * @return 
 */
int ksnStreamSendTo(ksnStreamClass *ks, char *to_peer, char *stream_name, 
        uint8_t cmd, void *data, size_t data_len) {
    
    size_t stream_name_len = strlen(stream_name) + 1;
    size_t sp_len = sizeof(struct stream_packet) + stream_name_len + data_len;
    struct stream_packet *sp = malloc(sp_len);
    if(data == NULL) data_len = 0;
    sp->cmd = cmd;
    sp->data_len = data_len;
    sp->stream_name_len = stream_name_len;
    memcpy(sp->data, stream_name, sp->stream_name_len);
    if(data_len > 0) memcpy(sp->data + stream_name_len, data, data_len);
        
    ksnCoreSendCmdto(kev->kc, to_peer, CMD_STREAM, sp, sp_len);
    free(sp);
    
    return 0;
}

/**
 * Create new named stream with selected peer
 * 
 * @param ks
 * @param to_peer
 * @param stream_name
 * @param send_f
 *        CMD_ST_CREATE - create pipes and send CREATE request to peer; 
 *        CMD_ST_CREATE_GOT - CREATE command received, create pipes and send CREATED response  
 *        CMD_ST_CREATED - CREATED command received
 * 
 * @return 
 */
int ksnStreamCreate(ksnStreamClass *ks, char *to_peer, char *stream_name, 
        int send_f) {
    
    size_t to_peer_len = strlen(to_peer) + 1;
    size_t stream_name_len = strlen(stream_name) + 1;
    size_t key_buf_len = to_peer_len + stream_name_len;
    char key_buf[key_buf_len];
    
    memcpy(key_buf, to_peer, to_peer_len);
    memcpy(key_buf + to_peer_len, stream_name, stream_name_len);
    
    // Send create or created command
    if(send_f == CMD_ST_CREATE || send_f == CMD_ST_CREATE_GOT) {
        
        struct sream_data data;
        data.created = 0;
        int rc;

        if(pipe(data.pipe_in) == -1) return -1; // Can't create pipe
        if(pipe(data.pipe_out) == -1) return -1; // Can't create pipe

        // Add record to stream map
        if((rc = pblMapAdd(ks->map, (void*)key_buf, key_buf_len, &data, 
                sizeof(data))) >= 0) {
        
            // Send command CREATE to streams peer
            if(send_f == CMD_ST_CREATE) {
                
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG, 
                        "%sStream:%s " 
                        "Send CREATE stream name \"%s\" to peer \"%s\" ...\n", 
                        ANSI_BLUE, ANSI_NONE,
                        stream_name, to_peer);
                #endif

                ksnStreamSendTo(ks, to_peer, stream_name, CMD_ST_CREATE, 
                        NULL, 0);
            }
            else {
                
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG, 
                    "%sStream:%s "
                    "Got CREATE stream name \"%s\" request from peer \"%s\" ...\n", 
                    ANSI_BLUE, ANSI_NONE,
                    stream_name, to_peer);
                #endif

                // \todo Connect FDs
                
                ksnStreamSendTo(ks, to_peer, stream_name, CMD_ST_CREATED, 
                        NULL, 0);                
            }
        }
        else if(rc == 0) return -2; // This stream already exist
        
    } 
    
    // Got created command
    else if(send_f == CMD_ST_CREATED) {
        
        size_t valueLen;
        struct sream_data *data;        
        if((data = pblMapGet(ks->map, (void*)key_buf, key_buf_len, &valueLen))
                != NULL) {        
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&kev->ksn_cfg, DEBUG, 
                "%sStream:%s "
                "Got CREATED stream name \"%s\" from peer \"%s\" ...\n", 
                ANSI_BLUE, ANSI_NONE,
                stream_name, to_peer);
            #endif
            
            data->created = 1;
            
            // \todo Connect FDs
            
            // Send created event to application
            if(kev->event_cb != NULL)
                    kev->event_cb(kev, EV_K_STREAM_CONNECTED, (void*)key_buf, 
                            key_buf_len, NULL);
        }
        else return -3; // This stream was not requested
    }
    
    return 0;
}

/**
 * Process CMD_STREAM teonet command
 *
 * @param ks
 * @param rd
 * @return
 */
int cmd_stream_cb(ksnStreamClass *ks, ksnCorePacketData *rd) {
    
    int retval = 1;
    struct stream_packet *data = rd->data;
    
    switch(data->cmd) {
        
        case CMD_ST_CREATE:
            ksnStreamCreate(kev->ks, rd->from, data->data,  CMD_ST_CREATE_GOT);
            break;
            
        case CMD_ST_CREATED:
            ksnStreamCreate(kev->ks, rd->from, data->data,  CMD_ST_CREATED);
            break;
            
        default:
            retval = 0;
            break;
    }
    
    return retval; // Command processed
}

#undef kev
