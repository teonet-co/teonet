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

/**
 * Stream data
 */
struct sream_data {
    
//    char *name; ///< Stream name
//    char *peer; ///< Peer name
    int created; ///< Created flag
    int pipe_in[2]; ///< Input (read) pipe
    int pipe_out[2]; ///< Output (write) pipe
    
};

/**
 * Stream subcommands
 */
enum stream_cmd {
    
    CMD_ST_CREATE,      ///< Create stream command
    CMD_ST_CREATE_GOT,  ///< Got create request
    CMD_ST_CREATED,     ///< Stream created (answer to create) command
    CMD_ST_DATA,        ///< Send data command
    CMD_ST_CLOSE        ///< Close stream command
};

/**
 * Stream packet structure
 */
struct stream_packet {
    
    uint8_t cmd; ///< Stream subcommand
    uint16_t data_len; ///< Data len
    uint8_t stream_name_len; ///< Stream name len
    char data[]; ///< Stream data: stream_name + stream data
};

/**
 * Stream module class structure
 */
typedef struct ksnStreamClass {
    
    void *ke;       ///< Pointer to ksnetEvMgrClass
    PblMap* map;    ///< Stream map
    
} ksnStreamClass;

#ifdef	__cplusplus
extern "C" {
#endif

ksnStreamClass *ksnStreamInit(void *ke);
void ksnStreamDestroy(ksnStreamClass *ks);
int ksnStreamCreate(ksnStreamClass *ks, char *to_peer, char *stream_name, 
        int send_f);


#ifdef	__cplusplus
}
#endif

#endif	/* STREAM_H */
