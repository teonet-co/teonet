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

#include "trudp.h"

#define make_addr(addr_str, port, addr, addrlen) trudpUdpMakeAddr(addr_str, port, addr, addrlen)

#ifdef	__cplusplus
extern "C" {
#endif

ssize_t ksnTRUDPrecvfrom(trudpData *td, int fd, void *buffer, size_t buffer_len, 
        int flags, __SOCKADDR_ARG addr, socklen_t *addr_len);

ssize_t ksnTRUDPsendto(trudpData *td, int resend_fl, uint32_t id, int attempt,
        int cmd, int fd, const void *buf, size_t buf_len, int flags,
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len);

void trudp_event_cb(void *tcd_pointer, int event, void *data,
        size_t data_length, void *user_data);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_TR_UDP_H */

