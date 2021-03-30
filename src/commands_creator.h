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

typedef struct connect_r_packet {
    uint32_t port;
    uint8_t ip_counts;
    char type[64];
    char ips[];
} connect_r_packet_t;

uint8_t* createCmdConnectRPacketUdp(ksnetEvMgrClass *event_manager, size_t *size_out);
uint8_t* createCmdConnectRPacketTcp(ksnetEvMgrClass *event_manager, size_t *size_out);

#endif