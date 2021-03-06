/**
 * File:   net_split.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on July 17, 2015, 10:20 AM
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "net_split.h"
#include "utils/rlutil.h"
#include "utils/teo_memory.h"

#define MODULE _ANSI_BLUE "net_split" _ANSI_NONE

#define kev ((ksnetEvMgrClass*)(((ksnCoreClass*)(ks->kco->kc))->ke))

/**
 * Initialize split module
 *
 * @param kco
 * @return
 */
ksnSplitClass *ksnSplitInit(ksnCommandClass *kco) {

    ksnSplitClass *ks = teo_malloc(sizeof(ksnSplitClass));
    ks->kco = kco;
    ks->map = pblMapNewHashMap();
    ks->packet_number = 0;
    ks->last_added = 0;

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
 * @param cmd
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

        packets = malloc(PACKETS_AR_LEN);

        for(i = 0; i < *num_subpackets; i++) {

            packets[i] = malloc(MAX_DATA_LEN + sizeof(uint16_t) * 3 + (!i ? sizeof(uint8_t):0));

            int is_last = i == *num_subpackets - 1;
            size_t ptr = 0;
            uint16_t data_len = is_last ? packet_len - MAX_DATA_LEN * i : MAX_DATA_LEN; // Calculate data length
            *(uint16_t*)(packets[i]) = i ? data_len : data_len + sizeof(uint8_t); ptr += sizeof(uint16_t); // Set Data length in buffer
            *(uint16_t*)(packets[i] + ptr) = (uint16_t) packet_number; ptr += sizeof(uint16_t); // Packet number
            *(uint16_t*)(packets[i] + ptr) = (uint16_t) i | (is_last ? 0x8000 : 0); ptr += sizeof(uint16_t); // Subpacket number
            memcpy(packets[i] + ptr, packet + MAX_DATA_LEN * i, data_len); ptr += data_len; // Subpacket data
            if(!i) {
                *(uint8_t*)(packets[i] + ptr) = cmd; // Add CMD to data
            }
        }

        #ifdef DEBUG_KSNET
        ksn_printf(kev, MODULE, DEBUG_VV,
            "%d bytes packet was split to %d subpackets\n",
            (int)packet_len, *num_subpackets);
        #endif
    }

    return packets;
}

/**
 * Combine packets to one large packet
 *
 * @param ks Pointer to ksnSplitClass
 * @param rd Pointer to ksnCorePacketData
 *
 * @return Pointer to combined packet ksnCorePacketData or NULL if not combined
 *         or error
 */
ksnCorePacketData *ksnSplitCombine(ksnSplitClass *ks, ksnCorePacketData *rd) {

    ksnCorePacketData *rds = NULL;
    double current_time = ksnetEvMgrGetTime(kev);

    // Parse command
    size_t ptr_d = 0;
    uint16_t packet_num = *(uint16_t*)rd->data; ptr_d += sizeof(uint16_t); // Packet number
    int last_packet = (*(uint16_t*)(rd->data + ptr_d)) & LAST_PACKET_FLAG; // Is this last packet
    uint16_t subpacket_num = (*(uint16_t*)(rd->data + ptr_d)) & (LAST_PACKET_FLAG-1); ptr_d += sizeof(uint16_t); // Subpacket number

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
    void *key = teo_malloc(key_len); \
    size_t ptr = 0; \
    *(uint8_t*)key = rd->from_len; ptr += sizeof(uint8_t); \
    memcpy(key + ptr, rd->from, rd->from_len); ptr += rd->from_len; \
    *(uint16_t*)(key + ptr) = packet_num ; ptr += sizeof(packet_num); \
    *(uint16_t*)(key + ptr) = subpacket_num ; ptr += sizeof(subpacket_num)

    // Clear all map raws if time from last adding is longe than 10 sec
    if(current_time - ks->last_added > 10.0 && !pblMapIsEmpty(ks->map)) {
        pblMapClear(ks->map);
    }

    // Add subpacket to map
    create_key(subpacket_num);
    pblMapAdd(ks->map, key, key_len, rd->data + ptr_d, rd->data_len - ptr_d);
    free(key);

    // Save current time when record was added to map
    ks->last_added = current_time;

    // Combine subpackets to large packet and create ksnCorePacketData
    if(last_packet) {

        // Create memory buffer for combined block
        size_t data_len = 0, data_len_alloc = MAX_DATA_LEN * 2;
        void *data = malloc(data_len_alloc);

        // Create new rds
        rds = teo_calloc(sizeof(ksnCorePacketData));

        // Get subpackets from map and add it to combined block
        int i;
        for(i = 0; i <= subpacket_num; i++) {

            // Get subpacket from map
            create_key(i);
            size_t data_s_len;
            void *data_s = pblMapGet(ks->map, key, key_len, &data_s_len);

            // Check error (the subpacket has not received or added to the map)
            if(data_s == NULL) {
                #ifdef DEBUG_KSNET
                ksn_puts(kev, MODULE, ERROR_M,
                    "the subpacket has not received or added to the map\n");
                free(key);
                #endif
                return NULL;
            }

            // Get command from first subpacket
            if(!i) {
                rds->cmd = *(uint8_t*)(data_s + (data_s_len - sizeof(uint8_t)));
                data_s_len -= sizeof(uint8_t);
            }

            // Reallocate memory
            if(data_len_alloc < data_len + data_s_len) {
                data_len_alloc = data_len + data_s_len;
                data = teo_realloc(data, data_len_alloc);
            }

            // Combine data
            memcpy(data + data_len, data_s, data_s_len);
            data_len += data_s_len;

            // Remove subpacket from map
            pblMapRemoveFree(ks->map, key, key_len, &data_s_len);
            free(key);
       }

        #ifdef DEBUG_KSNET
        ksn_printf(kev, MODULE, DEBUG_VV,
            "combine %d subpackets to large %d bytes packet\n",
            subpacket_num+1, (int)data_len);
        #endif

        // Create new ksnCorePacketData for combined block
        rds->addr = rd->addr;
        rds->arp = rd->arp;
        rds->data = data;
        ks->data_save = data;
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
void ksnSplitFreeRds(ksnSplitClass *ks, ksnCorePacketData *rd) {

    if(rd != NULL) {
        free(ks->data_save);
        free(rd);
    }
}

#undef create_key
#undef kev
