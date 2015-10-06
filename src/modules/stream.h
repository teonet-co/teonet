/** 
 * File:   stream.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet stream module
 *
 * Created on October 3, 2015, 3:02 PM
 */

#ifndef STREAM_H
#define	STREAM_H

#include <stdint.h>
#include <stdio.h>
#include <pbl.h>
#include <ev.h>

/**
 * Stream map data structure
 */
typedef struct ksnStreamMapData {
    
    int created;        ///< Created flag
    int pipe_in[2];     ///< Input (read) pipe
    ev_io w_in;         ///< Input stream watcher
    int pipe_out[2];    ///< Output (write) pipe
    ev_io w_out;        ///< Output stream watcher
    void *ke;           ///< Pointer to ksnetEvMgrClass (to use in watchers)
    void *key;          ///< This key (copy)
    size_t key_len;     ///< Key length
    
} ksnStreamMapData;

/**
 * Stream map data structure
 */
typedef struct ksnStreamData {
    
    const char *stream_name;
    const char *peer_name;   
    int fd_in;
    int fd_out;
    
} ksnStreamData;

/**
 * Stream subcommands
 */
enum stream_cmd {
    
    CMD_ST_NONE,
    CMD_ST_CREATE,          ///< Create stream command
    CMD_ST_CREATE_GOT,      ///< Got create request
    CMD_ST_CREATED,         ///< Stream created (answer to create) command
    CMD_ST_DATA,            ///< Send data command
    CMD_ST_CLOSE,           ///< Close stream command
    CMD_ST_CLOSE_GOT,       ///< Got close request from stream
    CMD_ST_CLOSE_NOTREMOVE  ///< Close stream but not remove from stream map
};

/**
 * Stream packet data structure
 */
struct stream_packet {
    
    uint8_t cmd;        ///< Stream subcommand
    uint16_t data_len;  ///< Data len
    uint8_t stream_name_len; ///< Stream name len
    char data[];        ///< Stream data: stream_name + stream data
};

/**
 * Stream module class structure
 */
typedef struct ksnStreamClass {
    
    void *ke;           ///< Pointer to ksnetEvMgrClass
    PblMap* map;        ///< Stream map
    
} ksnStreamClass;

#ifdef	__cplusplus
extern "C" {
#endif

ksnStreamClass *ksnStreamInit(void *ke);
void ksnStreamDestroy(ksnStreamClass *ks);
int ksnStreamCreate(ksnStreamClass *ks, char *to_peer, char *stream_name, 
        int send_f);
int ksnStreamClose(ksnStreamClass *ks, char *to_peer, char *stream_name, 
        int send_f);
int ksnStreamClosePeer(ksnStreamClass *ks, const char *peer_name);
ksnStreamData *ksnStreamGetDataFromMap(ksnStreamData *sd, ksnStreamMapData *smd);
ksnStreamMapData *ksnStreamGetMapData(ksnStreamClass *ks, void *key, size_t key_len);
/*
 * Get output stream FD
 * 
 * @param ks Pointer to ksnStreamClass
 * @param key Key buffer
 * @param key_len Key buffer length
 * 
 * @return Output stream FD
 */
//#define ksnStreamGetOutFd(ks, key, key_len) ksnStreamGetMapData(ks, key, key_len)->pipe_out[1]
/*
 * Get input stream FD
 * 
 * @param ks Pointer to ksnStreamClass
 * @param key Key buffer
 * @param key_len Key buffer length
 * 
 * @return Output stream FD
 */
//#define ksnStreamGetInFd(ks, key, key_len) ksnStreamGetData(ks, key, key_len)->pipe_in[0]
//#define ksnStreamGetInFd(sd) sd->pipe_in[0]

#ifdef	__cplusplus
}
#endif

#endif	/* STREAM_H */
