/**
* \file commands_creator.c
* \author max
* Created on Mon Mar 29 17:14:47 2021
*/

#include "commands_creator.h"

uint8_t* createCmdConnectRPacketUdp(ksnetEvMgrClass *event_manager, size_t *size_out) {
    // Create data with list of local IPs and port
    ksnet_stringArr ips = getIPs(&event_manager->teo_cfg); // IPs array

    uint8_t len = ksnet_stringArrLength(ips); // Max number of IPs
    const size_t MAX_IP_STR_LEN = 16; // Max IPs string length

    *size_out = sizeof(connect_r_packet_t) + len * MAX_IP_STR_LEN;
    connect_r_packet_t *packet = malloc(*size_out);

    packet->ip_counts = 0;
    packet->port = event_manager->kc->port;

    const char* type = teoGetAppType(event_manager);

    size_t hid_len;
    host_info_data *hid = teoGetHostInfo(event_manager, &hid_len);

    if(hid != NULL) {
        char* full_type = teoGetFullAppTypeFromHostInfo(hid);
        strncpy(packet->type, full_type, sizeof(packet->type));
        free(hid);
    } else {
        strncpy(packet->type, type, sizeof(packet->type));
    }

    size_t ptr = 0;

    // Fill data with IPs and Port
    for(int i=0; i < len; i++) {
        if(ip_is_private(ips[i])) {
            int ip_len =  strlen(ips[i]) + 1;
            memcpy(packet->ips + ptr, ips[i], ip_len);
            ptr += ip_len;
            packet->ip_counts++;
        }

    }

    ksnet_stringArrFree(&ips);

    return (uint8_t*)packet;
}


uint8_t* createCmdConnectRPacketTcp(ksnetEvMgrClass *event_manager, size_t *size_out) {
    // Create data with empty list of local IPs and port
    uint8_t* packet = malloc(sizeof(uint8_t));
    uint8_t *num = (uint8_t *) packet; // Pointer to number of IPs
    *size_out = sizeof(uint8_t); // Pointer (to first IP)
    *num = 0; // Number of IPs

    return packet;
}