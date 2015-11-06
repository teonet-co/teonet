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

// Local functions
void connect_watchers(ksnStreamClass *ks, ksnStreamMapData *data, 
        void *key_buf, size_t key_buf_len);
int ksnStreamCloseAll(ksnStreamClass *ks);

#define kev ((ksnetEvMgrClass*)ks->ke)

#define KSN_STREAM_CONNECT_TIMEOUT 5.000

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
        ksnStreamCloseAll(ks);
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
int ksnStreamSendTo(ksnStreamClass *ks, const char *to_peer, const char *stream_name, 
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
        
    ksnCoreSendCmdto(kev->kc, (char*)to_peer, CMD_STREAM, sp, sp_len);
    free(sp);
    
    return 0;
}

/**
 * Callback Queue callback (the same as callback queue event). 
 * 
 * This function calls at timeout or after ksnCQueExec calls
 * 
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback 
 * @param data Pointer to ksnStreamMapData
 */
void kq_connect_cb(uint32_t id, int type, void *data) {
    
    ksnStreamMapData *smd = data;
    ksnetEvMgrClass *ke = smd->ke;
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
        "%sStream:%s " 
        "Got connect CQueue callback with id: %d, type: %d => %s\n", 
        ANSI_BLUE, ANSI_NONE,
        id, type, type ? "success" : "timeout");
    #endif

    // Remove Stream if timeout occurred
    if(!type) {
        
        // Send timeout created event to application
        if(ke->event_cb != NULL)
            ke->event_cb(ke, EV_K_STREAM_CONNECT_TIMEOUT, smd, 
                    sizeof(ksnStreamMapData), NULL); 
        
        // Remove this stream
        ksnStreamClose(ke->ks, smd->key, smd->key + strlen(smd->key) + 1, 
                CMD_ST_CLOSE_GOT);
    }
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
        
        ksnStreamMapData data;
        data.key = malloc(key_buf_len);
        memcpy(data.key, key_buf, key_buf_len);
        data.key_len = key_buf_len;
        data.created = 0;
        data.ke = kev;
        data.cq = NULL;
        int rc;

        if(pipe(data.pipe_in) == -1) return -1; // Can't create pipe
        if(pipe(data.pipe_out) == -1) return -1; // Can't create pipe

        // Add record to stream map
        if((rc = pblMapAdd(ks->map, (void*)key_buf, key_buf_len, &data, 
                sizeof(data))) >= 0) {
        
            size_t valueLen;
            ksnStreamMapData *data;        
            if((data = pblMapGet(ks->map, (void*)key_buf, key_buf_len, &valueLen))
                != NULL) {
            
                // Send command CREATE to streams peer
                if(send_f == CMD_ST_CREATE) {

                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                        "%sStream:%s " 
                        "Send CREATE stream name \"%s\" to peer \"%s\" ...\n", 
                        ANSI_BLUE, ANSI_NONE,
                        stream_name, to_peer);
                    #endif

                    // Sen CREATE request to remote peer
                    ksnStreamSendTo(ks, to_peer, stream_name, CMD_ST_CREATE, 
                            NULL, 0);
                    
                    // Add callback to Teonet queue with 5 sec timeout
                    data->cq = ksnCQueAdd(kev->kq, kq_connect_cb, 
                            KSN_STREAM_CONNECT_TIMEOUT, data);
                }

                // Create stream watchers, connect it to pipes and send CREATED 
                // signal to input stream (to stream initiator)
                else {

                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                        "%sStream:%s "
                        "Got CREATE stream name \"%s\" request from peer \"%s\" ...\n", 
                        ANSI_BLUE, ANSI_NONE,
                        stream_name, to_peer);
                    #endif

                    // Create watchers and connect it to pipe FDs
                    connect_watchers(ks, data, key_buf, key_buf_len);

                    ksnStreamSendTo(ks, to_peer, stream_name, CMD_ST_CREATED, 
                            NULL, 0);                
                }
            }
        }
        else if(rc == 0) return -2; // This stream already exist        
    } 
    
    // Got created response
    else if(send_f == CMD_ST_CREATED) {
        
        size_t valueLen;
        ksnStreamMapData *data;        
        if((data = pblMapGet(ks->map, (void*)key_buf, key_buf_len, &valueLen))
                != NULL) {        
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                "%sStream:%s "
                "Got CREATED stream name \"%s\" from peer \"%s\" ...\n", 
                ANSI_BLUE, ANSI_NONE,
                stream_name, to_peer);
            #endif
                        
            // Connect FDs
            connect_watchers(ks, data, key_buf, key_buf_len);    
            
            // Send success to CQUEUE
            ksnCQueExec(kev->kq, data->cq->id);
        }
        else return -3; // This stream was not requested
    }
    
    return 0;
}

/**
 * Close named stream with selected peer
 * 
 * @param ks
 * @param to_peer
 * @param stream_name
 * @param send_f
 * @return 
 */
int ksnStreamClose(ksnStreamClass *ks, char *to_peer, char *stream_name, 
        int send_f) {
    
    size_t to_peer_len = strlen(to_peer) + 1;
    size_t stream_name_len = strlen(stream_name) + 1;
    size_t key_buf_len = to_peer_len + stream_name_len;
    char key_buf[key_buf_len];    
    memcpy(key_buf, to_peer, to_peer_len);
    memcpy(key_buf + to_peer_len, stream_name, stream_name_len);
    
    size_t valueLen;
    ksnStreamMapData *smd;        
    if((smd = pblMapGet(ks->map, (void*)key_buf, key_buf_len, &valueLen))
        != NULL) {
                
        // Close stream
        if(smd->created) {
            
            // Stop watchers
            ev_io_stop(kev->ev_loop, &smd->w_in);
            ev_io_stop(kev->ev_loop, &smd->w_out);
            
            // Send close request to peer
            if(send_f != CMD_ST_CLOSE_GOT) {

                ksnStreamSendTo(ks, to_peer, stream_name, CMD_ST_CLOSE, NULL, 0);   
            }
            
            // Send disconnected event to application
            if(kev->event_cb != NULL)
                kev->event_cb(kev, EV_K_STREAM_DISCONNECTED, key_buf, 
                        key_buf_len, NULL);
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                "%sStream:%s "
                "Stream with name \"%s\" was disconnected from peer \"%s\" ...\n", 
                ANSI_BLUE, ANSI_NONE,
                stream_name, to_peer);
            #endif
            
        }
        
        // Free stream map data record
        free(smd->key);
        if(send_f != CMD_ST_CLOSE_NOTREMOVE)
            pblMapRemoveFree(ks->map, (void*)key_buf, key_buf_len, &valueLen);
    }
    
    return 0;
}

/**
 * Close all streams
 * 
 * @param ks
 * @return 
 */
int ksnStreamCloseAll(ksnStreamClass *ks) {
    
    PblIterator *it =  pblMapIteratorNew(ks->map);
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *key = pblMapEntryKey(entry);
            ksnStreamClose(ks, key, key + strlen(key) + 1, CMD_ST_CLOSE_NOTREMOVE);
        }
        pblIteratorFree(it);
    }
    pblMapClear(ks->map);
    
    return 0;
}

/**
 * Close all streams connected to selected peer
 * 
 * @param ks
 * @param peer_name
 * @return 
 */
int ksnStreamClosePeer(ksnStreamClass *ks, const char *peer_name) {
    
    PblIterator *it =  pblMapIteratorNew(ks->map);
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *key = pblMapEntryKey(entry);
            if(!strcmp(key, peer_name)) {
                ksnStreamClose(ks, key, key + strlen(key) + 1, CMD_ST_CLOSE_GOT);
                ksnStreamClosePeer(ks, peer_name);
                break;
            }
        }
        pblIteratorFree(it);
    }
    
    return 0;
}

/**
 * Get stream map data by key
 * 
 * @param ks
 * @param key
 * @param key_len
 * 
 * @return Pointer to ksnStreamMapData or NULL if not found
 */
ksnStreamMapData *ksnStreamGetMapData(ksnStreamClass *ks, void *key, 
        size_t key_len) {
    
    ksnStreamMapData *data;      
    size_t valueLen;
    
    if((data = pblMapGet(ks->map, (void*)key, key_len, &valueLen)) != NULL) {
        
    }
    
    return data;
}

/**
 * Get stream data
 * 
 * @param sd Pointer to ksnStreamData
 * @param smd Pointer to ksnStreamMapData
 * @return 
 */
ksnStreamData *ksnStreamGetDataFromMap(ksnStreamData *sd, ksnStreamMapData *smd) {
    
    sd->stream_name = (char*)smd->key + strlen(smd->key) + 1; // Stream name
    sd->peer_name = (char*)smd->key; // Peer name
    sd->fd_out = smd->pipe_out[1]; // Output stream FD
    sd->fd_in = smd->pipe_in[0]; // Input stream FD
    
    return sd;
}    

/**
 * Input stream (has data) callback
 *
 * @param loop
 * @param w
 * @param revents
 */
void stream_in_cb (struct ev_loop *loop, /*EV_P_*/ ev_io *w, int revents) {
    
    ksnStreamMapData *data = w->data;
    ksnetEvMgrClass *ke = data->ke;
    
    // Send DATA event to application
    if(ke->event_cb != NULL)
        ke->event_cb(ke, EV_K_STREAM_DATA, data, sizeof(ksnStreamMapData), 
                NULL);
}

/**
 * Output stream (has data) callback
 *
 * @param loop
 * @param w
 * @param revents
 */
void stream_out_cb (struct ev_loop *loop, /*EV_P_*/ ev_io *w, int revents) {
    
    ksnStreamMapData *data = w->data;
    ksnetEvMgrClass *ke = data->ke;
    
    const size_t buf_len = KSN_BUFFER_SM_SIZE;
    char buf[buf_len];
    ssize_t rc = read(data->pipe_out[0], buf, buf_len);
    if(rc >= 0) {
        const char *to_peer = data->key;
        const char *stream_name = data->key + strlen(data->key) + 1;
        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
            "%sStream:%s "
            "Sent %d byte DATA to stream name \"%s\" to peer \"%s\" ...\n", 
            ANSI_BLUE, ANSI_NONE,
            (int) rc,
            stream_name, to_peer);
        #endif
        ksnStreamSendTo(ke->ks, to_peer, stream_name, CMD_ST_DATA, buf, rc);    
    }
}

/**
 * Connect watchers to pipes fd
 * 
 * @param ks
 * @param data
 * @param key_buf
 * @param key_buf_len
 */
void connect_watchers(ksnStreamClass *ks, ksnStreamMapData *data, 
        void *key_buf, size_t key_buf_len) {
    
    // Pipe IN
    ev_init (&data->w_in, stream_in_cb);
    ev_io_set (&data->w_in, data->pipe_in[0], EV_READ);
    data->w_in.data = data;
    ev_io_start (kev->ev_loop, &data->w_in);

    // Pipe OUT
    ev_init (&data->w_out, stream_out_cb);
    ev_io_set (&data->w_out, data->pipe_out[0], EV_READ);
    data->w_out.data = data;
    ev_io_start (kev->ev_loop, &data->w_out);
    
    data->created = 1;
    
    // Send created event to application
    if(kev->event_cb != NULL)
        kev->event_cb(kev, EV_K_STREAM_CONNECTED, key_buf, key_buf_len, NULL);    
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
        
        // CREATE request
        case CMD_ST_CREATE:
            ksnStreamCreate(kev->ks, rd->from, data->data,  CMD_ST_CREATE_GOT);
            break;
            
        // CREATED response
        case CMD_ST_CREATED:
            ksnStreamCreate(kev->ks, rd->from, data->data,  CMD_ST_CREATED);
            break;
            
        // CLOSE request
        case CMD_ST_CLOSE:
            ksnStreamClose(kev->ks, rd->from, data->data, CMD_ST_CLOSE_GOT);
            break;
            
        // Got a data
        case CMD_ST_DATA:
        {
            size_t valueLen;
            ksnStreamMapData *sd;
            size_t peer_name_len = strlen(rd->from) + 1;
            size_t stream_name_len = strlen(data->data) + 1;
            size_t key_buf_len = peer_name_len + stream_name_len;
            
            char key_buf[key_buf_len];
            memcpy(key_buf, rd->from, peer_name_len);
            memcpy(key_buf + peer_name_len, data->data, stream_name_len);            
            if((sd = pblMapGet(ks->map, (void*)key_buf, key_buf_len, &valueLen))
                != NULL) { 
                
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                    "%sStream:%s "
                    "Got %d byte DATA to stream name \"%s\" from peer \"%s\" ...\n", 
                    ANSI_BLUE, ANSI_NONE,
                    data->data_len,
                    data->data, rd->from);
                #endif

                // Write stream data to Input(read) pipe
                if(write(sd->pipe_in[1], data->data + data->stream_name_len, 
                        data->data_len) >= 0);
            }
        } break;
            
        default:
            retval = 0;
            break;
    }
    
    return retval; // Command processed
}

#undef kev
