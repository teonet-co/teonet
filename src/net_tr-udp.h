/** 
 * File:   net_tr-udp.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet Real time communications over UDP protocol (TR-UDP)
 *
 * Created on August 4, 2015, 12:16 AM
 */

#ifndef NET_TR_UDP_H
#define	NET_TR_UDP_H

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
    tr_udp_stat stat; ///< TR-UDP Statistic data

} ksnTRUDPClass;

#ifdef	__cplusplus
extern "C" {
#endif


// Main functions
ksnTRUDPClass *ksnTRUDPinit(void *kc);
void ksnTRUDPDestroy(ksnTRUDPClass *tu);

ssize_t ksnTRUDPsendto(ksnTRUDPClass *tu, int resend_fl, uint32_t id, 
        int cmd, int attempt, int fd, const void *buf, size_t buf_len, int flags, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len);
ssize_t ksnTRUDPrecvfrom(ksnTRUDPClass *tu, int fd, void *buf, size_t buf_len,
        int flags, __SOCKADDR_ARG addr, socklen_t *addr_len);

void ksnTRUDPresetAddr(ksnTRUDPClass *tu, const char *addr, int port, int options);


#ifdef	__cplusplus
}
#endif

#endif	/* NET_TR_UDP_H */

