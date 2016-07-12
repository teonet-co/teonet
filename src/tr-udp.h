/**
 * File:   tr-udp.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet Real time communications over UDP protocol (TR-UDP)
 *
 * Created on August 4, 2015, 12:16 AM
 */

#ifndef NET_TR_UDP_H
#define	NET_TR_UDP_H

#define TRUDV_VERSION 2

#if TRUDV_VERSION == 1

#ifdef HAVE_MINGW
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
typedef int socklen_t;
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include <stdio.h>
#include <pbl.h>


/**
 * registerProcessPacket callback function type definition
 */
typedef void (*ksnTRUDPprocessPacketCb) (void *kc, void *buf, size_t recvlen,
              __SOCKADDR_ARG remaddr);

/**
 * TR-UDP Statistic data
 */
typedef struct tr_udp_stat {

    struct send_list {
        size_t size_max;
        size_t size_current;
        size_t attempt;
    } send_list;

    struct receive_heap {
        size_t size_max;
        size_t size_current;
    } receive_heap;

} tr_udp_stat;

/**
 * Teonet TR-UDP class data
 */
typedef struct ksnTRUDPClass {

    void *kc; ///< Pointer to KSNet core class object
    PblMap *ip_map; ///< IP:port map
    tr_udp_stat stat; ///< TR-UDP Common statistic data
    ksnTRUDPprocessPacketCb process_packet; ///< TR-UDP recvfrom Process Packet function
    double started; ///< Start module time
    ev_io write_w; ///< Write watcher
    int write_w_init_f; ///< Set to true after initialize

} ksnTRUDPClass;

#ifdef	__cplusplus
extern "C" {
#endif


// Main functions
ksnTRUDPClass *ksnTRUDPinit(void *kc);
void ksnTRUDPDestroy(ksnTRUDPClass *tu);

void ksnTRUDPremoveAll(ksnTRUDPClass *tu);

ssize_t ksnTRUDPsendto(ksnTRUDPClass *tu, int resend_fl, uint32_t id, int attempt,
        int cmd, int fd, const void *buf, size_t buf_len, int flags,
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len);
ssize_t ksnTRUDPrecvfrom(ksnTRUDPClass *tu, int fd, void *buf, size_t buf_len,
        int flags, __SOCKADDR_ARG addr, socklen_t *addr_len);

void *ksnTRUDPregisterProcessPacket(ksnTRUDPClass *tu, ksnTRUDPprocessPacketCb pc);

void ksnTRUDPresetAddr(ksnTRUDPClass *tu, const char *addr, int port, int options);

void *ksnTRUDPstatGet(ksnTRUDPClass *tu, int type, size_t *stat_len);


#ifdef	__cplusplus
}
#endif

#endif

#if TRUDV_VERSION == 2

#define make_addr(addr_str, port, addr, addrlen) trudpUdpMakeAddr(addr_str, port, addr, addrlen)

#ifdef	__cplusplus
extern "C" {
#endif

ssize_t ksnTRUDPrecvfrom(void *td, int fd, void *buffer,
                         size_t buffer_len, int flags, __SOCKADDR_ARG addr,
                         socklen_t *addr_len);

ssize_t ksnTRUDPsendto(void *td, int resend_fl, uint32_t id, int attempt,
        int cmd, int fd, const void *buf, size_t buf_len, int flags,
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len);

void trudp_event_cb(void *tcd_pointer, int event, void *data,
        size_t data_length, void *user_data);

#ifdef	__cplusplus
}
#endif

#endif

#endif	/* NET_TR_UDP_H */

