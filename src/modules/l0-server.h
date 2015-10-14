/** 
 * File:   l0-server.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on October 8, 2015, 1:39 PM
 */

#ifndef L0_SERVER_H
#define	L0_SERVER_H

#include <pbl.h>
#include "../embedded/libteol0/teonet_l0_client.h"

/**
 * L0 Server map data structure
 * 
 */
typedef struct ksnLNullData {
    
    ev_io w;                ///< TCP Client watcher
    char *name;             ///< Clients name
    size_t name_length;     ///< Clients name length
    void *read_buffer;      ///< Pointer to saved buffer
    size_t read_buffer_ptr; ///< Pointer in read buffer
    size_t read_buffer_size;///< Read buffer size

    
} ksnLNullData;  
                   
/**
 * ksnLNull Class structure definition
 */
typedef struct  ksnLNullClass {
    
    void *ke;       ///< Pointer to ksnEvMgrClass
    PblMap *map;    ///< Pointer to the L0 clients map (by fd)
    PblMap *map_n;  ///< Pointer to the L0 FDs map (by name)
    int fd;         ///< L0 TCP Server FD
    
} ksnLNullClass;

/**
 * L0 Server resend to peer packet data structure
 * 
 */        
typedef struct ksnLNullSPacket {

    uint8_t cmd; ///< Command
    uint8_t from_length; ///< From client name length (include leading zero)
    uint16_t data_length; ///< Packet data length
    char from[]; ///< From client name (include leading zero) + packet data

} ksnLNullSPacket;


#ifdef	__cplusplus
extern "C" {
#endif

ksnLNullClass *ksnLNullInit(void *ke);
void ksnLNullDestroy(ksnLNullClass *kl);
int ksnLNullSendToL0(void *ke, char *addr, int port, char *cname, 
        size_t cname_length, uint8_t cmd, void *data, size_t data_len);

#ifdef	__cplusplus
}
#endif

#endif	/* L0_SERVER_H */
