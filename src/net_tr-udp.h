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

/**
 * Teonet TR-UDP class data
 */
typedef struct ksnTRUDPClass {
    void *kc; ///< Pointer to KSNet core class object
    PblMap *ip_map; ///< IP:port map

} ksnTRUDPClass;

/**
 * Receive heap data
 */
typedef struct rh_data {
    uint32_t id;
    void *data;
    size_t data_len;
    struct sockaddr addr;
    socklen_t addr_len;

} rh_data;

#ifdef	__cplusplus
extern "C" {
#endif


// Main functions
ksnTRUDPClass *ksnTRUDPInit(void *kc);
void ksnTRUDPDestroy(ksnTRUDPClass *tu);

ssize_t ksnTRUDPsendto(ksnTRUDPClass *tu, int fd, int cmd, const void *buf,
        size_t buf_len, int flags, __CONST_SOCKADDR_ARG addr,
        socklen_t addr_len);
ssize_t ksnTRUDPrecvfrom(ksnTRUDPClass *tu, int fd, void *buf, size_t buf_len,
        int flags, __SOCKADDR_ARG addr, socklen_t *addr_len);


// Used in tests
int ksnTRUDPReceiveHeapCompare(const void* prev, const void* next);


#ifdef	__cplusplus
}
#endif

#endif	/* NET_TR_UDP_H */

