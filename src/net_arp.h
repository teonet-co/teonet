/**
 * File:   net_arp.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 12, 2015, 7:19 PM
 *
 * KSNet ARP Table manager
 */

#ifndef NETARP_H
#define	NETARP_H

#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>

#include <pbl.h>

#include "net_core.h"
#include "teonet_l0_client.h"

///**
// * KSNet ARP table data structure
// */
//typedef struct ksnet_arp_data {
//
//    int16_t mode;       ///< Peers mode: -1 - This host, -2 undefined host, 0 - peer , 1 - r-host, 2 - TCP Proxy peer
//    char addr[40];      ///< Peer IP address
//    int16_t port;       ///< Peer port
//
//    double last_acrivity;           ///< Last time receved data from peer
//    double last_triptime_send;      ///< Last time when triptime request send
//    double last_triptime_got;       ///< Last time when triptime received
//
//    double last_triptime;           ///< Last triptime
//    double triptime;                ///< Middle triptime
//
//    double monitor_time;            ///< Monitor ping time
//
//} ksnet_arp_data;
//
///**
// * KSNet ARP table whole data array
// */
//typedef struct ksnet_arp_data_ar {
//    
//    uint32_t length;
//    struct _arp_data {
//        
//        char name[40];
//        ksnet_arp_data data;
//        
//    } arp_data[];
//    
//} ksnet_arp_data_ar;

/**
 * KSNet ARP functions data
 */
typedef struct ksnetArpClass {

    PblMap* map;    ///< Hash Map to store KSNet ARP table
    void *ke;       ///< Pointer to Event Manager class object

} ksnetArpClass;


//typedef void (*peer_remove_cb_t)(ksnCoreClass *kn, char *name, size_t name_len, int ch);

#ifdef	__cplusplus
extern "C" {
#endif


// Peers ARP table functions
ksnetArpClass *ksnetArpInit(void *ke);
void ksnetArpDestroy(ksnetArpClass *ka);
void ksnetArpAdd(ksnetArpClass *ka, char* name, ksnet_arp_data *data);
void ksnetArpAddHost(ksnetArpClass *ka); 
void *ksnetArpSetHostPort(ksnetArpClass *ka, char* name, int port);
ksnet_arp_data *ksnetArpGet(ksnetArpClass *ka, char *name);
ksnet_arp_data *ksnetArpRemove(ksnetArpClass *ka, char* name);
void ksnetArpRemoveAll(ksnetArpClass *ka);
int ksnetArpShow(ksnetArpClass *ka);
char *ksnetArpShowStr(ksnetArpClass *ka);
int ksnetArpGetAll(ksnetArpClass *ka, int (*peer_callback)(ksnetArpClass *ka, char *peer_name, ksnet_arp_data *arp_data, void *data), void *data);
int ksnetArpGetAllH(ksnetArpClass *ka, int (*peer_callback)(ksnetArpClass *ka, char *peer_name, ksnet_arp_data *arp_data, void *data), void *data);
ksnet_arp_data *ksnetArpFindByAddr(ksnetArpClass *ka, __CONST_SOCKADDR_ARG addr);
ksnet_arp_data_ar *ksnetArpShowData(ksnetArpClass *ka);
size_t ksnetArpShowDataLength(ksnet_arp_data_ar *peers_data);
char *ksnetArpShowLine(int num, char *name, ksnet_arp_data* data);
char *ksnetArpShowHeader(int header_f);

#ifdef	__cplusplus
}
#endif

#endif	/* NETARP_H */
