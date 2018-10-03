/** 
 * File:   net_split.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on July 17, 2015, 10:20 AM
 * 
 * Module to split large packets when send and combine it when receive.
 * 
 */

#ifndef NET_SPLIT_H
#define	NET_SPLIT_H

#include "ev_mgr.h"

#define MAX_DATA_LEN 448
#define MAX_PACKET_LEN 0x7FFFF
#define LAST_PACKET_FLAG 0x8000

/**
 * KSNet split class data
 */
typedef struct ksnSplitClass {
    
    ksnCommandClass *kco;
    PblMap* map;    ///< Hash Map to store splitted packets
    uint16_t packet_number; ///< Large packet number
    double last_added; ///< Last time when record added to map
    
} ksnSplitClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnSplitClass *ksnSplitInit(ksnCommandClass *kc);
void ksnSplitDestroy(ksnSplitClass *ks);

void **ksnSplitPacket(ksnSplitClass *ks, uint8_t cmd, void *packet, size_t packet_len, int *num_subpackets);
ksnCorePacketData *ksnSplitCombine(ksnSplitClass *ks, ksnCorePacketData *rd);
void ksnSplitFreeRds(ksnSplitClass *ks, ksnCorePacketData *rd);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_SPLIT_H */
