/**
 * File:   net_tun.c
 * Author: Kirill Scherba
 *
 * Created on May 10, 2015, 3:27 PM
 *
 * KSNet network tunnel module
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "ev_mgr.h"

/**
 * Tunnel commands
 */
enum tun_command {
    TUN_CREATE,         ///< Create client connection to endpoint server
    TUN_CREATED,        ///< Connection to endpoint server created
    TUN_NOT_CREATED,    ///< Can't create connection to endpoint server
    TUN_DISCONNECTED,   ///< Connection disconnected (by endpoint client or server)
    TUN_DATA            ///< Tunnel data
};

/**
 * Tunnel server accept data
 */
struct ksn_tun_accept_data {

    ksnTunClass *ktun;
    uint16_t port;
    char *to;
    uint16_t to_port;
    char *to_ip;
};

// Local functions
void cmd_tun_create_cb(ksnTunClass * ktun, char *from, uint8_t from_len,
                       uint16_t fd, uint16_t port, char *ip);
void cmd_tun_disconnected_send(ksnTunClass *ktun, uint16_t fd);
void ksn_tun_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *watcher,
                       int revents, int fd);
void cmd_tun_created_cb(ksnTunClass * ktun, char *from, uint8_t from_len,
                        uint16_t fd, uint16_t from_fd);
void cmd_tun_disconnected_cb(ksnTunClass * ktun,
                        uint16_t fd);
void cmd_tun_read_cb (EV_P_ ev_io *w, int revents);
void cmd_tun_write_cb(uint16_t fd, void *data, size_t data_len);
//
void ksnTunMapRemove(ksnTunClass * ktun, uint16_t fd);
void ksnTunMapFree(ksnTunClass *ktun);
//
void ksnTunListAdd(ksnTunClass * ktun, int fd, struct ksn_tun_accept_data *tun_d);

/**
 * Initialize ksnet tunnel class
 *
 * @param ke
 * @return
 */
ksnTunClass *ksnTunInit(void *ke) {

    ksnTunClass *ktun = malloc(sizeof(ksnTunClass));
    ktun->list = pblMapNewHashMap();
    ktun->map = pblMapNewHashMap();
    ktun->ke = ke;

    return ktun;
}

/**
 * DeInitialize ksnet tunnel class and free its data structure
 *
 * @param kt
 * @return
 */
void ksnTunDestroy(ksnTunClass * ktun) {

    // Disconnect all connected FD
    ksnTunMapFree(ktun);
    pblMapFree(ktun->list);
    free(ktun);
}

/**
 * Create tunnel with peer port (and IP)
 *
 * @param ktun Pointer to ksnTunClass
 * @param to_port This host port
 * @param to Tunnel to peer name
 * @param to_port Tunnels remote port
 * @param to_ip Tunnels remote IP. If to_ip is NULL or empty string then connect to port at peers localhost
 */
void ksnTunCreate(ksnTunClass * ktun, uint16_t port, char *to,
                  uint16_t to_port, char *to_ip) {

    struct ksn_tun_accept_data *tun_d = malloc(sizeof(*tun_d));
    tun_d->port = port;
    tun_d->ktun = ktun;
    tun_d->to = strdup(to);
    tun_d->to_port = to_port;
    tun_d->to_ip = to_ip != NULL ? strdup(to_ip) : NULL;

    // Create TCP server at port, which will wait client connections
    int port_created,
        fd = ksnTcpServerCreate(((ksnetEvMgrClass*)ktun->ke)->kt, port,
                       ksn_tun_accept_cb, tun_d, &port_created);

    // Add created server to list. The server will be destroyed in ksnTcpServerAccept function
    if(fd > 0) {
        tun_d->port = port_created;
        ksnTunListAdd(ktun, fd, tun_d);
    }
}

/**
 * Tunnel server accept callback
 *
 * @param loop
 * @param watcher
 * @param revents
 * @param fd
 */
void ksn_tun_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *watcher,
                       int revents, int fd) {

    struct ksn_tun_accept_data *tun_d = watcher->data;

    // Create command data
    size_t ptr = 0,
           data_len = sizeof(uint8_t) + sizeof(uint16_t) * 2 +
                      (tun_d->to_ip != NULL ? strlen(tun_d->to_ip) : 0) + 1;

    char data[data_len];
    *((uint8_t *)data) = TUN_CREATE; ptr += sizeof(uint8_t); // Tunnel command
    *((uint16_t *)(data + ptr)) = (uint16_t)fd; ptr += sizeof(uint16_t); // FD
    *((uint16_t *)(data + ptr)) = (uint16_t)tun_d->to_port; ptr += sizeof(uint16_t); // Port
    if(tun_d->to_ip != NULL)
        strcpy((char *)(data + ptr), tun_d->to_ip);
    else
        strcpy((char *)(data + ptr), NULL_STR);  // IP

    // Send tunnel create command
    if(ksnCoreSendCmdto(((ksnetEvMgrClass *)(tun_d->ktun->ke))->kc, tun_d->to,
        CMD_TUN, data, data_len) == NULL) {

        // Close socket
        #ifdef DEBUG_KSNET
        ksnet_printf(
                & ((ksnetEvMgrClass *)(tun_d->ktun->ke))->ksn_cfg, DEBUG,
                "TUN Server: Can't create tunnel to %s, disconnect... \n",
                tun_d->to
        );
        #endif
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
    else {

        #ifdef DEBUG_KSNET
        ksnet_printf(
                & ((ksnetEvMgrClass *)(tun_d->ktun->ke))->ksn_cfg, DEBUG,
                "TUN Server: Tunnel to %s created \n",
                tun_d->to
        );
        #endif
    }
}

/******************************************************************************/
/* Tunnel map functions                                                       */
/******************************************************************************/

/**
 * Add new record to the tunnels map
 *
 * @param ktun
 * @param fd This host ft
 * @param to Peer name
 * @param to_len Peer name length
 * @param to_fd Peers FD
 */
void ksnTunMapAdd(ksnTunClass * ktun, uint16_t fd, char *to, uint8_t to_len,
                  uint16_t to_fd) {

    size_t ptr = 0, data_len = to_len + sizeof(uint16_t);
    char data[data_len];

    strcpy(data, to); ptr += to_len;
    *((uint16_t*)(data + ptr)) = to_fd;

    pblMapAdd(ktun->map, &fd, sizeof(fd), data, data_len);
}

/**
 * Find record in the tunnels map
 *
 * @param ktun
 * @param fd
 * @param to_fd
 * @return
 */
char *ksnTunMapGet(ksnTunClass *ktun, uint16_t fd, uint16_t *to_fd) {

    size_t val_len;
    void *data = pblMapGet(ktun->map, &fd, sizeof(fd), &val_len);

    if(data != NULL)
        *to_fd = * ((uint16_t*)(data + strlen(data) + 1));

    return data;
}

/**
 * Remove record from tunnels map
 *
 * @param ktun
 * @param fd
 */
void ksnTunMapRemove(ksnTunClass * ktun, uint16_t fd) {

    size_t val_len;
    pblMapRemove(ktun->map, &fd, sizeof(fd), &val_len);
}

/**
 * Close all connected FD and free tunnel map
 * @param ktun
 */
void ksnTunMapFree(ksnTunClass * ktun) {

    PblIterator *it = pblMapIteratorNew(ktun->map);
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            uint16_t *fd = (uint16_t *) pblMapEntryKey(entry);

            //printf("ksnTunMapFree, fd: %d\n", *fd);

            // Close socket
            //shutdown(*fd, SHUT_RDWR);
            close(*fd);

            cmd_tun_disconnected_send(ktun, *fd);
        }
    }

    pblMapFree(ktun->map);
}

/******************************************************************************/
/* Tunnel list functions                                                      */
/******************************************************************************/

/**
 * Add record to tunnels list
 *
 * @param ktun
 * @param fd
 * @param tun_d
 */
void ksnTunListAdd(ksnTunClass *ktun, int fd, struct ksn_tun_accept_data *tun_d) {

    pblMapAdd(ktun->list, &fd, sizeof(fd), tun_d, sizeof(*tun_d));
}

/**
 * Remove tunnel from list
 *
 * @param ktun
 * @param fd
 */
void *ksnTunListRemove(ksnTunClass *ktun, int fd) {

    size_t val_len;
    return pblMapRemove(ktun->list, &fd, sizeof(fd), &val_len);
}

/**
 * Show list (return string) of tunnels
 *
 * @param ktun
 * @return Null terminated string with list of peers, should be free after use.
 */
char *ksnTunListShow(ksnTunClass * ktun) {

    char *list; // = strdup(NULL_STR);

    list = ksnet_formatMessage(
        "\n"
        "List of created tunnels:\n"
        "\n"
        "--------------------------------------------------------------\n"
        "  FD   Port            Peer      Port      IP\n"
        "--------------------------------------------------------------\n"
    );

    PblIterator *it = pblMapIteratorNew(ktun->list);
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            uint16_t *fd = (uint16_t *) pblMapEntryKey(entry);
            struct ksn_tun_accept_data *tun_d = (struct ksn_tun_accept_data *) pblMapEntryValue(entry);

            list = ksnet_sformatMessage(list,
                "%s"
                "%4d %6d %15s    %6d      %-15s\n"
                , list, *fd, tun_d->port, tun_d->to, tun_d->to_port
                , tun_d->to_ip ? tun_d->to_ip : NULL_STR
            );
        }
    }

    pblIteratorFree(it);

    return list;
}

/******************************************************************************/
/* Tunnel commands callback                                                   */
/******************************************************************************/

/**
 * Process cmd_tun ksnet command
 *
 * @param kco
 * @param rd
 * @return
 */
int cmd_tun_cb(ksnTunClass * ktun, ksnCorePacketData *rd) {

    size_t ptr = 0;
    uint8_t *tun_cmd = (uint8_t *)rd->data; ptr += sizeof(*tun_cmd);
    uint16_t *from_fd = (uint16_t *)(rd->data + ptr); ptr += sizeof(*from_fd);

    switch(*tun_cmd) {

        // Parse TUN Create command
        case TUN_CREATE:
        {
            uint16_t *port = (uint16_t *)(rd->data + ptr); ptr += sizeof(*port);
            char *ip = (char *)(rd->data + ptr);
            cmd_tun_create_cb(ktun, rd->from, rd->from_len, *from_fd, *port, ip);
        }
        break;

        // Parse TUN Created command
        case TUN_CREATED:
        {
            uint16_t *fd = (uint16_t *)(rd->data + ptr); ptr += sizeof(*fd);
            cmd_tun_created_cb(ktun, rd->from, rd->from_len, *fd, *from_fd);
        }
        break;

        // Send received tunnel data to socket
        case TUN_DATA:
        {
            uint16_t *fd = from_fd;
            cmd_tun_write_cb(*fd, rd->data + ptr, rd->data_len - ptr);
        }
        break;

        // Parse TUN Disconnected command
        case TUN_DISCONNECTED:
        {
            uint16_t *fd = from_fd;
            cmd_tun_disconnected_cb(ktun, *fd);
        }
        break;

        // Parse TUN Created command
        case TUN_NOT_CREATED:
        {
            uint16_t *fd = from_fd;
            cmd_tun_disconnected_cb(ktun, *fd);
        }
        break;

        default:
            break;
    }

    return 1; // Command processed
}

/**
 * Read data from tunnel socket and resend it to peer
 *
 * @param loop
 * @param w
 * @param revents
 */
void cmd_tun_read_cb (EV_P_ ev_io *w, int revents) {

    const size_t BUF_LEN = KSN_BUFFER_SIZE + KSN_BUFFER_SM_SIZE;
    char buffer[BUF_LEN];
    size_t ptr = 0, tun_title_len = sizeof(uint8_t) + sizeof(uint16_t);

    // Read data from socket
    int read_len = read(w->fd, buffer + tun_title_len, BUF_LEN - tun_title_len);

    // Close connection
    if(!read_len) {

        #ifdef DEBUG_KSNET
        ksnet_printf(
            & ((ksnetEvMgrClass*)((ksnTunClass *)w->data)->ke)->ksn_cfg , DEBUG,
            "TUN Server: Connection closed. Stop listening FD %d\n",
            w->fd
        );
        #endif

        // Stop and free watcher if client socket is closing
        ev_io_stop(loop, w);

        // Close socket
        //shutdown(w->fd, SHUT_RDWR);
        close(w->fd);

        // Send disconnected to remote peer
        cmd_tun_disconnected_send((ksnTunClass *)w->data, w->fd);

        // Remove from map
        ksnTunMapRemove((ksnTunClass *)w->data, w->fd);

        // Free watcher memory
        free(w);

        return;
    }

    // TODO: Read error
    else if(read_len < 0) {

        #ifdef DEBUG_KSNET
        ksnet_printf(
            & ((ksnetEvMgrClass*)((ksnTunClass *)w->data)->ke)->ksn_cfg , DEBUG,
            "TUN Server : read error\n"
        );
        #endif

        return;
    }

    // Get w->fd data from tunnels map
    uint16_t to_fd;
    char *to = ksnTunMapGet(w->data, w->fd, &to_fd);

    // Send TUN_DATA command to peer
    if(to != NULL) {

        // Send answer: send TUN_DATA command
        *((uint8_t *)buffer) = TUN_DATA; ptr += sizeof(uint8_t); // Tunnel command
        *((uint16_t *)(buffer + ptr)) = (uint16_t)to_fd; ptr += sizeof(uint16_t); // FD
        ksnCoreSendCmdto(
            ((ksnetEvMgrClass *)(((ksnTunClass *)w->data)->ke))->kc, to, CMD_TUN,
            buffer, read_len + tun_title_len);
    }
}

/**
 * Write data to connected FD
 *
 * @param ktun
 * @param fd
 * @param data
 * @param data_len
 */
inline void cmd_tun_write_cb(uint16_t fd, void *data, size_t data_len) {

    write(fd, data, data_len);
}

/**
 * Create connection to endpoint server callback
 * @param fd
 *
 * @param ktun
 * @param from
 * @param from_len
 * @param from_fd
 * @param port
 * @param ip
 */
void cmd_tun_create_cb(ksnTunClass *ktun, char *from, uint8_t from_len,
                       uint16_t from_fd, uint16_t port, char *ip) {

    // Connect to endpoint server
    int fd = ksnTcpClientCreate(((ksnetEvMgrClass*)ktun->ke)->kt, port,
                       ip != NULL && ip[0] != '\0' ? ip : localhost);

    if(fd > 0) {

        // Create TUN_CREATED data
        size_t ptr = 0, data_len = sizeof(uint8_t) + sizeof(uint16_t) * 2;
        char data[data_len];
        *((uint8_t *)data) = TUN_CREATED; ptr += sizeof(uint8_t); // Tunnel command
        *((uint16_t *)(data + ptr)) = (uint16_t) fd; ptr += sizeof(uint16_t); // FD
        *((uint16_t *)(data + ptr)) = (uint16_t) from_fd; ptr += sizeof(uint16_t); // From FD

        // Send answer: send TUN_CREATED command
        ksnCoreSendCmdto(((ksnetEvMgrClass *)(ktun->ke))->kc, from, CMD_TUN,
            data, data_len);

        // Add record to tunnel map & Add FD to ksnet event manager
        cmd_tun_created_cb(ktun, from, from_len, fd, from_fd);
    }
    else {

        // Create TUN_NOT_CREATED data
        size_t ptr = 0, data_len = sizeof(uint8_t) + sizeof(uint16_t);
        char data[data_len];
        *((uint8_t *)data) = TUN_NOT_CREATED; ptr += sizeof(uint8_t); // Tunnel command
        *((uint16_t *)(data + ptr)) = (uint16_t) from_fd; ptr += sizeof(uint16_t); // From FD

        // Send answer: send TUN_NOT_CREATED command
        ksnCoreSendCmdto(((ksnetEvMgrClass *)(ktun->ke))->kc, from, CMD_TUN,
            data, data_len);
    }
}

/**
 * Send tunnel disconnected command
 *
 * @param ktun
 * @param from
 * @param from_len
 * @param from_fd
 */
void cmd_tun_disconnected_send(ksnTunClass *ktun, uint16_t fd) {

    uint16_t to_fd;
    char *to = ksnTunMapGet(ktun, fd, &to_fd);

    // Create TUN_NOT_CREATED data
    size_t ptr = 0, data_len = sizeof(uint8_t) + sizeof(uint16_t);
    char data[data_len];
    *((uint8_t *)data) = TUN_NOT_CREATED; ptr += sizeof(uint8_t); // Tunnel command
    *((uint16_t *)(data + ptr)) = (uint16_t) to_fd; ptr += sizeof(uint16_t); // From FD

    // Send answer: send TUN_CREATED command
    ksnCoreSendCmdto(((ksnetEvMgrClass *)(ktun->ke))->kc, to, CMD_TUN,
        data, data_len);
}

/**
 * Connection with endpoint server was established
 *
 * @param ktun
 * @param from
 * @param from_len
 * @param fd
 * @param from_fd
 */
void cmd_tun_created_cb(ksnTunClass * ktun, char *from, uint8_t from_len,
                        uint16_t fd, uint16_t from_fd) {

    // Add record to tunnel map
    ksnTunMapAdd(ktun, fd, from, from_len, from_fd);

    // Add FD to ksnet event manager
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wstrict-aliasing"

    // Create and start watcher (start client processing)
    ev_io *w = malloc(sizeof(*w));
    ev_init (w, cmd_tun_read_cb);
    ev_io_set (w, fd, EV_READ);
    w->data = ktun;
    ev_io_start (((ksnetEvMgrClass*)ktun->ke)->ev_loop, w);
    #pragma GCC diagnostic pop
}

/**
 * Connection with endpoint server was not established
 *
 * @param ktun
 * @param fd
 */
void cmd_tun_disconnected_cb(ksnTunClass * ktun, uint16_t fd) {

    // Close FD
    shutdown(fd, SHUT_RDWR);
    //close(fd);
}
