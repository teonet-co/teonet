/**
* \file commands_creator.h
* \author max
* Created on Mon Mar 29 17:14:47 2021
*/

#ifndef COMMANDS_CREATOR_H
#define COMMANDS_CREATOR_H

#include <stdint.h>
#include <stdlib.h>

#include "ev_mgr.h"

#pragma pack(push)
#pragma pack(1)
typedef struct connect_r_packet {
    uint32_t    port;
    uint8_t     ip_counts;
    char        type[64];
    char        ips[];
} connect_r_packet_t;

typedef struct connect_packet {
    uint32_t    port;   ///< Peer port
    char        *name;  ///< Peer name
    char        *addr;  ///< Peer IP address
} connect_packet_t;

#pragma pack(pop)

uint8_t* createCmdConnectRPacketUdp(ksnetEvMgrClass *event_manager, size_t *size_out);
uint8_t* createCmdConnectRPacketTcp(ksnetEvMgrClass *event_manager, size_t *size_out);
uint8_t* createCmdConnectPacket(ksnetEvMgrClass *event_manager, size_t *size_out);

#endif