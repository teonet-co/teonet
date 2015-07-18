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
void **ksnSplitPacket(ksnSplitClass *ks, uint8_t cmd, void *packet, size_t packet_len, int *num_subpackets) {

    *num_subpackets = 0;
    void **packets = NULL;

    if(packet_len > MAX_DATA_LEN) {

        int i;
        uint16_t packet_number = ks->packet_number++;

        *num_subpackets = packet_len / MAX_DATA_LEN + ((packet_len % MAX_DATA_LEN) ? 1:0);
        const size_t PACKETS_AR_LEN = sizeof(void*) * (*num_subpackets);
 //       printf("ksnSplitPacket: start split %d bytes packet to %d subpackets\n", (int)packet_len, *num_subpackets);

        packets = malloc(PACKETS_AR_LEN);
//        printf("ksnSplitPacket: size of packets: %d\n", (int)PACKETS_AR_LEN);

        for(i = 0; i < *num_subpackets; i++) {

//            printf("ksnSplitPacket: before malloc\n");
            packets[i] = malloc(MAX_DATA_LEN + sizeof(uint16_t) * 3 + (!i ? sizeof(uint8_t):0));

//            printf("ksnSplitPacket: malloc packets[%d]\n", i);
//
            int is_last = i == *num_subpackets - 1;
            size_t ptr = 0;
            uint16_t data_len = is_last ? packet_len - MAX_DATA_LEN * i : MAX_DATA_LEN; // Calculate data length
            *(uint16_t*)(packets[i]) = i ? data_len : data_len + sizeof(uint8_t); ptr += sizeof(uint16_t); // Set Data length in buffer
//            printf("ksnSplitPacket: 0\n");
            *(uint16_t*)(packets[i] + ptr) = (uint16_t) packet_number; ptr += sizeof(uint16_t); // Packet number
//            printf("ksnSplitPacket: 1\n");
            *(uint16_t*)(packets[i] + ptr) = (uint16_t) i | (is_last ? 0x8000 : 0); /* printf("ksnSplitPacket: subpacket %d\n", *(uint16_t*)(packets[i] + ptr)); */ ptr += sizeof(uint16_t); // Subpacket number
            memcpy(packets[i] + ptr, packet + MAX_DATA_LEN * i, data_len); ptr += data_len; // Subpacket data
//            printf("ksnSplitPacket: 3\n");
            if(!i) {
                *(uint8_t*)(packets[i] + ptr) = cmd; // Add CMD to data
//                printf("ksnSplitPacket: cmd = %d\n", cmd);
            }
//            printf("ksnSplitPacket: 4\n");
        }

        printf("ksnSplitPacket: %d bytes packet was splitted to %d subpackets\n", (int)packet_len, *num_subpackets);
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
    uint16_t packet_num = *(uint16_t*)rd->data; ptr_d += sizeof(uint16_t); // Packet number
    int last_packet = (*(uint16_t*)(rd->data + ptr_d)) & (MAX_PACKET_LEN + 1); // Is this last packet
    uint16_t subpacket_num = (*(uint16_t*)(rd->data + ptr_d)) & MAX_PACKET_LEN; ptr_d += sizeof(uint16_t); // Subpacket number

    /* Map key structure:
     *
     * uint8_t  from_length
     * char[]   from
     * uint16_t packet_num
     * uint16_t sub_packet_num
     */

    // Create maps key macros
    #define create_key(subpacket_num) \
    size_t key_len = sizeof(uint8_t) + rd->from_len + sizeof(uint16_t)*2; \
    void *key = malloc(key_len); \
    size_t ptr = 0; \
    *(uint8_t*)key = rd->from_len; ptr += sizeof(uint8_t); \
    memcpy(key + ptr, rd->from, rd->from_len); ptr += rd->from_len; \
    *(uint16_t*)(key + ptr) = packet_num ; ptr += sizeof(packet_num); \
    *(uint16_t*)(key + ptr) = subpacket_num ; ptr += sizeof(subpacket_num)

    // Add subpacket to map
    create_key(subpacket_num);
    pblMapAdd(ks->map, key, key_len, rd->data + ptr_d, rd->data_len - ptr_d);
    free(key);
//    printf("ksnSplitCombine: add subpacket %d to map\n", subpacket_num);

    // Combine subpackets to large packet and create ksnCorePacketData
    if(last_packet) {

        // Create memory buffer for combined block
        size_t data_len = 0, data_len_alloc = MAX_DATA_LEN * 2;
        void *data = malloc(data_len_alloc);

        // Create new rds
        rds = malloc(sizeof(ksnCorePacketData));

        // Get subpackets from map and add it to combined block
        int i;
        for(i = 0; i <= subpacket_num; i++) {

            // Get subpacket from map
            create_key(i);
            size_t data_s_len;
            void *data_s = pblMapGet(ks->map, key, key_len, &data_s_len);
            free(key);

            // Check error (the subpacket has not received or added to the map)
            if(data_s == NULL) return NULL;

            // Get command from first subpacket
            if(!i) {
                rds->cmd = *(uint8_t*)(data_s + (data_s_len - sizeof(uint8_t)));
//                printf("ksnSplitCombine: rds->cmd = %d\n", rds->cmd);
                data_s_len -= sizeof(uint8_t);
            }

            // Reallocate memory
            if(data_len_alloc < data_len + data_s_len) {
                data_len_alloc = data_len + data_s_len;
                data = realloc(data, data_len_alloc);
            }

            // Combine data
            memcpy(data + data_len, data_s, data_s_len);
            data_len += data_s_len;
        }

        printf("ksnSplitCombine: combine %d subpackets to large %d bytes packet\n", subpacket_num+1, (int)data_len);

        // Create new ksnCorePacketData for combined block
        rds->addr = rd->addr;
        rds->arp = rd->arp;
        rds->data = data;
        rds->data_len = data_len;
        rds->from = rd->from;
        rds->from_len = rd->from_len;
        rds->mtu = rd->mtu;
        rds->port = rd->port;
        rds->raw_data = NULL;
        rds->raw_data_len = 0;
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

        free(rd->data);
        free(rd);
    }
}

#undef create_key
