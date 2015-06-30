/*
 * File:   net_core.h
 * Author: Kirill Scherba
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
#if KSNET_CRYPT
#include "crypt.h"
#endif

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

ksnCoreClass *ksnCoreInit(void* ke, char *name, int port, char* addr);
void ksnCoreDestroy(ksnCoreClass *kc);

int ksnCoreSendto(ksnCoreClass *kc, char *addr, int port, uint8_t cmd, void *data, size_t data_len);
ksnet_arp_data *ksnCoreSendCmdto(ksnCoreClass *kc, char *to, uint8_t cmd, void *data, size_t data_len);
#define ksnCoreSetEventTime(kc) kc->last_check_event = ksnetEvMgrGetTime(kc->ke)

#ifdef	__cplusplus
}
#endif

#endif	/* NET_CORE_H */
