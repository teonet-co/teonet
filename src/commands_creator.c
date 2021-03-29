/**
* \file commands_creator.c
* \author max
* Created on Mon Mar 29 17:14:47 2021
*/

#include "commands_creator.h"

typedef struct connect_r_packet {
    uint32_t port;
    char type[64];
    uint8_t ip_counts;
    char ips[];
} connect_r_packet_t;

uint8_t* createCmdConnectRPacketUdp(ksnetEvMgrClass *event_manager, size_t *size_out) {

}