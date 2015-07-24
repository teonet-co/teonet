/**
 * File:   net_tcp.h
 * Author: Kirill Scherba
 *
 * Created on March 29, 2015, 6:43 PM
 * Updated to use in libksmesh on May 06, 2015, 21:55
 * Adapted to use in libteonet on July 24, 2015, 11:56
 */

#ifndef NET_TCP_H
#define	NET_TCP_H

//#include "ev_mgr.h"
//extern int total_clients; // Total number of connected clients

/**
 * TCP Class data
 */
typedef struct ksnTcpClass  {

    void *ke;

} ksnTcpClass;

/**
 * KSNet event io
 */
typedef struct ev_ksnet_io {
    ev_io io;
    void (*ksnet_cb) (struct ev_loop *loop, struct ev_ksnet_io *watcher, int revents, int fd);
    void (*tcpServerAccept_cb) (struct ev_loop *loop, ev_io *w, int revents);
    int fd;
    int events;
    void *data;
} ev_ksnet_io;


#ifdef	__cplusplus
extern "C" {
#endif

ksnTcpClass *ksnTcpInit(void *ke);
void ksnTcpDestroy(ksnTcpClass *kt);

int ksnTcpServerCreate(ksnTcpClass *kt, int port, void (*ksnet_cb) (struct ev_loop *loop, struct ev_ksnet_io *watcher, int revents, int fd), void *data, int *port_created);
void ksnTcpCb(struct ev_loop *loop, int fd, void (*ksnet_read_cb)(struct ev_loop *loop, ev_io *watcher, int revents), void* data);

int ksnTcpClientCreate(ksnTcpClass *kt, int port, const char *server);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_TCP_H */
