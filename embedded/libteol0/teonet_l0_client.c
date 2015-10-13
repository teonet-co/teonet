/** 
 * File:   teonet_lo_client.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on October 12, 2015, 12:32 PM
 */

#include <stdint.h>
#include <string.h>

#include "teonet_l0_client.h"

/**
 * Create L0 client packet
 *  
 * @param buffer Buffer to create packet in
 * @param buffer_length Buffer length
 * @param command Command to peer
 * @param peer Teonet peer
 * @param data Command data
 * @param data_length Command data length
 * 
 * @return Length of packet
 */
size_t teoLNullPacketCreate(char* buffer, size_t buffer_length, 
        uint8_t command, char * peer, void* data, size_t data_length) {
    
    // \todo Check buffer length
    
    teoLNullCPacket* pkg = (teoLNullCPacket*) buffer;
    
    pkg->cmd = command;
    pkg->data_length = data_length;
    pkg->peer_name_length = strlen(peer) + 1;
    memcpy(pkg->peer_name, peer, pkg->peer_name_length);
    memcpy(pkg->peer_name + pkg->peer_name_length, data, pkg->data_length);
    pkg->checksum = teoByteChecksum(pkg->peer_name, pkg->peer_name_length + pkg->data_length);
    pkg->header_checksum = teoByteChecksum(pkg, sizeof(teoLNullCPacket) - sizeof(pkg->header_checksum));
    
    return sizeof(teoLNullCPacket) + pkg->peer_name_length + pkg->data_length;
}

/**
 * Create initialize L0 client packet
 * 
 * @param buffer Buffer to create packet in
 * @param buffer_length Buffer length
 * @param host_name Name of this L0 client
 * 
 * @return Pointer to teoLNullCPacket
 */
size_t teoLNullInit(char* buffer, size_t buffer_length, char* host_name) {
    
    return teoLNullPacketCreate(buffer, buffer_length, 0, "", host_name, 
            strlen(host_name) + 1);
}

/**
 * Calculate checksum
 * 
 * Calculate byte checksum in data buffer
 * 
 * @param data Pointer to data buffer
 * @param data_length Length of the data buffer to calculate checksum
 * 
 * @return Byte checksum of the input buffer
 */
uint8_t teoByteChecksum(void *data, size_t data_length) {
    
    int i;
    uint8_t *ch, checksum = 0;
    for(i = 0; i < data_length; i++) {
        ch = (uint8_t*)(data + i);
        checksum += *ch;
    }
    
    return checksum;
}
