/** 
 * File:   l0-server.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on October 8, 2015, 1:39 PM
 */

#ifndef L0_SERVER_H
#define	L0_SERVER_H

#include <pbl.h>

/**
 * L0 Server map data structure
 */
typedef struct ksnL0sData {
    
    ev_io w;            ///< TCP Client watcher
    char *name;         ///< Clients name
    size_t name_length; ///< Clients name length
    
} ksnL0sData;    
                   
/**
 * ksnCksnL0 Class structure definition
 */
typedef struct  ksnL0sClass {
    
    void *ke;       ///< Pointer to ksnEvMgrClass
    PblMap *map;    ///< Pointer to the L0 clients map (by fd))
    int fd;         ///< L0 TCP Server FD
    
} ksnL0sClass;

/**
 * L0 Server received from client packet data structure
 * 
 */        
typedef struct ksnL0sCPacket {

    uint8_t cmd; ///< Command
    uint8_t to_length; ///< To peer name length (include leading zero)
    uint16_t data_length; ///< Packet data length
    char to[]; ///< To peer name (include leading zero) + packet data

} ksnL0sCPacket;

/**
 * L0 Server resend to peer packet data structure
 * 
 */        
typedef struct ksnL0sSPacket {

    uint8_t cmd; ///< Command
    uint8_t from_length; ///< From client name length (include leading zero)
    uint16_t data_length; ///< Packet data length
    char from[]; ///< From client name (include leading zero) + packet data

} ksnL0sSPacket;


#ifdef	__cplusplus
extern "C" {
#endif

ksnL0sClass *ksnL0sInit(void *ke);
void ksnL0sDestroy(ksnL0sClass *kl);

#ifdef	__cplusplus
}
#endif

#endif	/* L0_SERVER_H */
