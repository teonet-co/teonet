/*
 * File:   net_core.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 28, 2015, 5:49 AM
 */

#ifndef NET_CORE_H
#define	NET_CORE_H

#include "config/config.h"

#include <ev.h>
#include <stdint.h>

#ifdef HAVE_MINGW
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#endif

#include "net_arp.h"
#include "net_com.h"
#include "tr-udp.h"

#if KSNET_CRYPT
#include "crypt.h"
#endif

#include "trudp.h"

// External constants
extern const char *localhost;
extern const char *null_str;

/**
 * KSNet mesh core data
 */
typedef struct ksnCoreClass {

    char *name;              ///< Host name
    uint8_t name_len;        ///< Host name length
    char *addr;              ///< Host IP address
    int port;                ///< Host IP port
    int fd;                  ///< Host socket file descriptor

    double last_check_event; ///< Last time of check host event
    ksnetArpClass *ka;       ///< Arp table class object
    ksnCommandClass *kco;    ///< Command class object
    trudpData *ku;          ///< TR-UDP class object
    #if KSNET_CRYPT
    ksnCryptClass *kcr;      ///< Crypt class object
    #endif
    ev_io host_w;            ///< Event Manager host (this host) watcher
    void *ke;                ///< Pointer to Event manager class object

    #ifdef HAVE_MINGW
    WSADATA wsaData;
    #endif

} ksnCoreClass;

#ifdef	__cplusplus
extern "C" {
#endif
    
/**
 * Sent teonet command to peer or l0 client
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param rd
 * @param name
 * @param out_data
 * @param out_data_len
 *
 * @return
 */
#define sendCmdAnswerTo(ke, rd, name, out_data, out_data_len) \
    if(rd->l0_f) \
        ksnLNullSendToL0(ke, \
            rd->addr, rd->port, name, strlen(name) + 1, rd->cmd, \
            out_data, out_data_len); \
    else \
        ksnCoreSendCmdto(ke->kc, name, rd->cmd, \
            out_data, out_data_len)    

ksnCoreClass *ksnCoreInit(void* ke, char *name, int port, char* addr);
void ksnCoreDestroy(ksnCoreClass *kc);

int ksnCoreSendto(ksnCoreClass *kc, char *addr, int port, uint8_t cmd, void *data, size_t data_len);
void teoBroadcastSend(ksnCoreClass *kc, char *to, uint8_t cmd, void *data, size_t data_len);
ksnet_arp_data *ksnCoreSendCmdto(ksnCoreClass *kc, char *to, uint8_t cmd, void *data, size_t data_len);
void ksnCoreProcessPacket (void *kc, void *buf, size_t recvlen,
        __SOCKADDR_ARG remaddr);
int ksnCoreParsePacket(void *packet, size_t packet_len, ksnCorePacketData *recv_data);
void ksnCoreCheckNewPeer(ksnCoreClass *kc, ksnCorePacketData *rd);
#define ksnCoreSetEventTime(kc) kc->last_check_event = ksnetEvMgrGetTime(kc->ke)

int ksnCoreBindRaw(int *port, int allow_port_increment_f);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_CORE_H */
