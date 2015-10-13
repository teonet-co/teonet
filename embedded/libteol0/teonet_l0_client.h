/** 
 * File:   teonet_l0_client.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on October 12, 2015, 12:32 PM
 */

#ifndef TEONET_L0_CLIENT_H
#define	TEONET_L0_CLIENT_H

#include <stdint.h>

/**
 * L0 client packet data structure
 * 
 */        
typedef struct teoLNullCPacket {

    uint8_t cmd; ///< Command
    uint8_t peer_name_length; ///< To peer name length (include leading zero)
    uint16_t data_length; ///< Packet data length
    uint8_t reserved_1; ///< Reserved 1
    uint8_t reserved_2; ///< Reserved 2
    uint8_t checksum; ///< Whole checksum
    uint8_t header_checksum; ///< Header checksum
    char peer_name[]; ///< To/From peer name (include leading zero) + packet data

} teoLNullCPacket;


#ifdef	__cplusplus
extern "C" {
#endif

int teoLNullClientCreate(int port, const char *server);
size_t teoLNullInit(char* buffer, size_t buffer_length, const char* host_name);
size_t teoLNullPacketCreate(char* buffer, size_t buffer_length, uint8_t command, 
        const char * peer, const void* data, size_t data_length);
uint8_t teoByteChecksum(void *data, size_t data_length);

#ifdef	__cplusplus
}
#endif

#endif	/* TEONET_L0_CLIENT_H */
