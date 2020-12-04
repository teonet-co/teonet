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
void ksnetArpAdd(ksnetArpClass *ka, char* name, ksnet_arp_data_ext *data);
void ksnetArpAddHost(ksnetArpClass *ka); 
void *ksnetArpSetHostPort(ksnetArpClass *ka, char* name, int port);
ksnet_arp_data_ext *ksnetArpGet(ksnetArpClass *ka, char *name);
int ksnetArpSize(ksnetArpClass *ka);
int ksnetArpRemove(ksnetArpClass *ka, char* name);
void ksnetArpRemoveAll(ksnetArpClass *ka);
int ksnetArpShow(ksnetArpClass *ka);
char *ksnetArpShowStr(ksnetArpClass *ka);
int ksnetArpGetAll_(ksnetArpClass *ka, int (*peer_callback)(ksnetArpClass *ka, char *peer_name, ksnet_arp_data_ext *arp_data, void *data), void *data, int flag);
int ksnetArpGetAll(ksnetArpClass *ka, int (*peer_callback)(ksnetArpClass *ka, char *peer_name, ksnet_arp_data_ext *arp_data, void *data), void *data);
int ksnetArpGetAllH(ksnetArpClass *ka, int (*peer_callback)(ksnetArpClass *ka, char *peer_name, ksnet_arp_data_ext *arp_data, void *data), void *data);
ksnet_arp_data *ksnetArpFindByAddr(ksnetArpClass *ka, __CONST_SOCKADDR_ARG addr, char **peer_name);

ksnet_arp_data_ar *ksnetArpShowData(ksnetArpClass *ka);
ksnet_arp_data_ext_ar *teoArpGetExtendedArpTable(ksnetArpClass *ka);
char *ksnetArpShowDataJson(ksnet_arp_data_ar *peers_data, size_t *peers_data_json_len);
char *teoArpGetExtendedArpTable_json(ksnet_arp_data_ext_ar *peers_data, size_t *peers_data_json_len);
void teoArpGetExtendedArpTable_json_delete(char *obj);
size_t ksnetArpShowDataLength(ksnet_arp_data_ar *peers_data);
size_t teoArpGetExtendedArpTableLength(ksnet_arp_data_ext_ar *peers_data);
#define ARP_TABLE_DATA_LENGTH(X) _Generic((X), \
      ksnet_arp_data_ar* : ksnetArpShowDataLength, \
      ksnet_arp_data_ext_ar* : teoArpGetExtendedArpTableLength \
      ) (X)

char *ksnetArpShowLine(int num, char *name, ksnet_arp_data* data);
char *ksnetArpShowHeader(int header_f);
void ksnetArpMetrics(ksnetArpClass *ka);

#ifdef	__cplusplus
}
#endif

#endif	/* NETARP_H */
