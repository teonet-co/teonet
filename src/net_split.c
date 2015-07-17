/** 
 * File:   net_split.c
 * Author: Kirill Scherba
 *
 * Created on July 17, 2015, 10:20 AM
 * 
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "net_split.h" 

/**
 * Initialize split module
 * 
 * @param kc
 * @return 
 */
ksnSplitClass *ksnSplitInit(ksnCommandClass *kc) {
    
    ksnSplitClass *ks = malloc(sizeof(ksnSplitClass));
    ks->kc = kc;  
    ks->map = pblMapNewHashMap();
    ks->packet_number = 0;
    
    return ks;
}

/**
 * Destroy split module
 * @param ks
 */
void ksnSplitDestroy(ksnSplitClass *ks) {
    
    if(ks != NULL) {
        
        pblMapFree(ks->map);
        free(ks);
    }
}

/**
 * Split large packet to array of small
 * 
 * @param ks
 * @param packet
 * @param packet_len
 * @param num_subpackets
 * @return 
 */
void **ksnSplitPacket(ksnSplitClass *ks, void *packet, size_t packet_len, int *num_subpackets) {
    
    *num_subpackets = 0;
    void **packets = NULL;    
    
    if(packet_len > MAX_DATA_LEN) {
        
        int i;
        uint16_t packet_number = ks->packet_number++;
                
        *num_subpackets = packet_len / MAX_DATA_LEN + (packet_len % MAX_DATA_LEN) ? 1:0;
        packets = malloc(sizeof(void*) * *num_subpackets);
        
        for(i = 0; i < *num_subpackets; i++) {
            
            packets[i] = malloc(sizeof(MAX_DATA_LEN) + sizeof(uint16_t) * 3);
            
            int is_last = i == *num_subpackets - 1;
            size_t ptr = 0;
            uint16_t data_len = is_last ? packet_len - MAX_DATA_LEN * i : MAX_DATA_LEN; // Calculate data length
            *(uint16_t*)(packets[i]) = data_len; ptr += sizeof(uint16_t); // Set Data length in buffer
            *(uint16_t*)(packets[i] + ptr) = (uint16_t) packet_number; ptr += sizeof(uint16_t); // Packet number
            *(uint16_t*)(packets[i] + ptr) = (uint16_t) i & is_last ? 0x8000 : 0; ptr += sizeof(uint16_t); // Subpacket number 
            memcpy(packets[i] + ptr, packet + MAX_DATA_LEN * i, data_len); // Subpacket data
        }
    }
    
    return packets;
}

/**
 * Combine packets to one large packet
 * 
 * @param ks
 * @param rd
 */
ksnCorePacketData *ksnSplitCombine(ksnSplitClass *ks, ksnCorePacketData *rd) {
    
    ksnCorePacketData *rds = NULL;
    
    /** TODO:
     * 
     * - Add packet to map if it is not last
     * - Combine saved to map packets to one lage if there is last packet
     */
    
    // Parse command
    size_t ptr_d = 0;
    char *from = rd->from; // From
    uint16_t packet_num = *(uint16_t*)rd->data; ptr_d += sizeof(uint16_t); // Packet number
    int last_packet = *(uint16_t*)(rd->data + ptr_d) & (MAX_PACKET_LEN + 1); // Is this last packet
    uint16_t subpacket_num = *(uint16_t*)(rd->data + ptr_d) & MAX_PACKET_LEN; ptr_d += sizeof(uint16_t); // Subpacket number
    
    /* Map key structure:
     * 
     * uint8_t  from_length
     * char[]   from
     * uint16_t packet_num 
     * uint16_t sub_packet_num 
     */ 
    
    // Add subpacket to map
    if(!last_packet) {
        
        // Create maps key
        size_t key_len = sizeof(uint8_t) + rd->from_len + sizeof(uint16_t)*2;
        void *key = malloc(key_len);
        size_t ptr = 0;
        *(uint8_t*)key = rd->from_len; ptr += sizeof(uint8_t); // from length
        memcpy(key + ptr, rd->from, rd->from_len); ptr += rd->from_len; // from
        *(uint16_t*)(key + ptr) = packet_num ; ptr += sizeof(packet_num); // packet number
        *(uint16_t*)(key + ptr) = subpacket_num ; ptr += sizeof(subpacket_num); // subpacket number

        // Add subpacket to map
        pblMapAdd(ks->map, key, key_len, rd->data + ptr_d, rd->data_len - ptr_d);
    }
    
    // TODO: Combine subpackets to large packet and create ksnCorePacketData
    else {
        
        // Create memory buffer for combined block
        
        // Get subpackets from map and add it to combined block
        
        // Create new ksnCorePacketData for combined block
        
    }
    
    return rds;
}

/**
 * Free ksnCorePacketData created in ksnSplitCombine function
 * 
 * @param ks
 * @param rd
 */
void ksnSplitFreRds(ksnSplitClass *ks, ksnCorePacketData *rd) {
    
    if(rd != NULL) {
        
        free(rd);
    }
}
