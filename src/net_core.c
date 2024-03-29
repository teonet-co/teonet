/**
 * KSNet mesh core module
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config/config.h"

#include <ev.h>
#ifdef HAVE_MINGW
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
typedef int socklen_t;
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include "ev_mgr.h"
#include "net_split.h"
#include "net_multi.h"
#include "net_com.h"
#include "utils/utils.h"
#include "utils/rlutil.h"
#include "utils/teo_memory.h"
#include "tr-udp.h"

#include "commands_creator.h"

// Constants
const char *localhost = "::1";//"127.0.0.1";
#define PACKET_HEADER_ADD_SIZE 2    // Sizeof from length + Sizeof command

#define MODULE _ANSI_GREEN "net_core" _ANSI_GREY
#define kev ((ksnetEvMgrClass *)teo_cfg->ke)

// Local functions
void host_cb(EV_P_ ev_io *w, int revents);
int ksnCoreBind(ksnCoreClass *kc);
void *ksnCoreCreatePacket(ksnCoreClass *kc, uint8_t cmd, const void *data, size_t data_len, size_t *packet_len);
void *ksnCoreCreatePacketFrom(ksnCoreClass *kc, uint8_t cmd, char *from, size_t from_len, const void *data,
        size_t data_len, size_t *packet_len);
int send_cmd_disconnect_peer_cb(ksnetArpClass *ka, char *name, ksnet_arp_data_ext *arp_data, void *data);
int send_cmd_disconnect_cb(ksnetArpClass *ka, char *name, ksnet_arp_data_ext *arp_data, void *data);
int send_cmd_connected_cb(ksnetArpClass *ka, char *name, ksnet_arp_data *arp_data, void *data);

// UDP / UDT functions
#define ksn_socket(domain, type, protocol) \
            socket(domain, type, protocol)

#define ksn_bind(fd, addr, addr_len) \
            bind(fd, addr, addr_len)

#define ksn_sendto(tu, cmd, fd, data, data_len, flags, remaddr, addrlen) \
            ksnTRUDPsendto(tu, 0, 0, 0, cmd, fd, data, data_len, flags, remaddr, addrlen)

#define ksn_recvfrom(ku, fd, buf, buf_len, flags, remaddr, addrlen) \
            ksnTRUDPrecvfrom(ku, fd, buf, buf_len, flags, remaddr, addrlen)

/**
 * Encrypt packet and sent to
 *
 * @param kc
 * @param cmd
 * @param DATA
 * @param D_LEN
 * @return
 */
#if KSNET_CRYPT
#define sendto_encrypt(kc, cmd, DATA, D_LEN) \
    { \
        if(((ksnetEvMgrClass*)kc->ke)->teo_cfg.crypt_f) { \
            size_t data_len; \
            char *buffer = NULL; /*[KSN_BUFFER_DB_SIZE];*/ \
            void *data = ksnEncryptPackage(kc->kcr, DATA, D_LEN, buffer, &data_len); \
            retval = ksn_sendto(kc->ku, cmd, kc->fd, data, data_len, 0, \
                                (struct sockaddr *)&remaddr, addrlen); \
            free(data); \
        } \
        else { \
            retval = ksn_sendto(kc->ku, cmd, kc->fd, DATA, D_LEN, 0, \
                                (struct sockaddr *)&remaddr, addrlen); \
        } \
    }
#else
#define sendto_encrypt(kc, cmd, DATA, D_LEN) \
    retval = ksn_sendto(kc->ku, cmd, kc->fd, DATA, D_LEN, 0, \
                        (struct sockaddr *)&remaddr, addrlen);
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

/**
 * Initialize ksnet core. Create socket FD and Bind ksnet UDP client/server
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param name Host name
 * @param port Host port. 0 - create system dependent port number
 * @param addr Host IP. NULL - listen at all IPs
 *
 * @return Pointer to ksnCoreClass or NULL if error
 */
ksnCoreClass *ksnCoreInit(void* ke, char *name, int port, char* addr) {
    ksnCoreClass *kc = teo_malloc(sizeof(ksnCoreClass));

    kc->ke = ke;
    kc->name = teo_strdup(name);
    kc->name_len = strlen(name) + 1;
    kc->port = port;
    if(addr != NULL) kc->addr = teo_strdup(addr);
    else kc->addr = addr;
    kc->last_check_event = 0;

    ((ksnetEvMgrClass*)ke)->kc = kc;
    kc->ka = ksnetArpInit(ke);
    kc->kco = ksnCommandInit(kc);
    #if KSNET_CRYPT
    kc->kcr = ksnCryptInit(ke);
    #endif

    // Create and bind host socket
    if(ksnCoreBind(kc)) {

        return NULL;
    }

    // TR-UDP initialize
    kc->ku = trudpInit(kc->fd, kc->port, trudp_event_cb, ke);

    // Change this host port number to port changed in ksnCoreBind function
    ksnetArpSetHostPort(kc->ka, ((ksnetEvMgrClass*)ke)->teo_cfg.host_name, kc->port);

    // Add host socket to the event manager
    if(!((ksnetEvMgrClass*)ke)->teo_cfg.r_tcp_f) {

        ev_io_init(&kc->host_w, host_cb, kc->fd, EV_READ);
        kc->host_w.data = kc;
        // TODO: The code moved to the idle_cb function. Remove this comment 
        // after some releases
        // ev_io_start(((ksnetEvMgrClass*)ke)->ev_loop, &kc->host_w);
    }

    return kc;
}

#pragma GCC diagnostic pop

/**
 * Close socket and free memory
 *
 * @param kc Pointer to ksnCore class object
 */
void ksnCoreDestroy(ksnCoreClass *kc) {
    if(kc != NULL) {
        ksnetEvMgrClass *ke = kc->ke;

        // Send disconnect from peer connected to TCP Proxy command to all
        ksnetArpGetAll(kc->ka, send_cmd_disconnect_peer_cb, NULL);

        // Send disconnect to all
        ksnetArpGetAll(kc->ka, send_cmd_disconnect_cb, NULL);

        // Stop watcher
        ev_io_stop(((ksnetEvMgrClass*)ke)->ev_loop, &kc->host_w);

        close(kc->fd);
        free(kc->name);
        if(kc->addr != NULL) free(kc->addr);
        ksnetArpDestroy(kc->ka);
        ksnCommandDestroy(kc->kco);
        trudpChannelDestroyAll(kc->ku);
        trudpDestroy(kc->ku);
        #if KSNET_CRYPT
        ksnCryptDestroy(kc->kcr);
        #endif
        free(kc);
        ke->kc = NULL;

        #ifdef HAVE_MINGW
        WSACleanup();
        #endif
    }
}

/**
 * Create and bind UDP socket for client/server
 *
 * @param[in] teo_cfg Pointer to teonet configuration: teonet_cfg
 * @param[out] port Pointer to Port number
 * @return File descriptor or error if return value < 0
 */
int ksnCoreBindRaw(int *port, int allow_port_increment_f) {
    return trudpUdpBindRaw(port, allow_port_increment_f);
}

/**
 * Create and bind UDP socket for client/server
 *
 * @param kc Pointer to ksnCoreClass
 * @return 0 if successfully created and bind
 */
int ksnCoreBind(ksnCoreClass *kc) {

    int fd;
    teonet_cfg *teo_cfg = & ((ksnetEvMgrClass*)kc->ke)->teo_cfg;

    #ifdef DEBUG_KSNET
    ksn_printf(kev, MODULE, DEBUG_VV,
            "create UDP client/server at port %d ...\n", kc->port);
    #endif

    if((fd = ksnCoreBindRaw(&kc->port, teo_cfg->port_inc_f)) > 0) {

        kc->fd = fd;
        #ifdef DEBUG_KSNET
        ksn_printf(((ksnetEvMgrClass*)kc->ke), MODULE, MESSAGE,
                "started at UDP port %d, socket fd %d\n", kc->port, kc->fd);
        #endif

        // Set non block mode
        // \todo Test with "Set non block on"
        //       and set the TEOSOCK_NON_BLOCKING_MODE if it work correct
        // It was tested and switch back to blocked UDP 2018-05-28
        //teosockSetBlockingMode(fd, TEOSOCK_NON_BLOCKING_MODE);
    }

    return !(fd > 0);
}

/**
 * Send data to remote peer IP:Port
 *
 * @param kc Pointer to KSNet core class object
 * @param addr IP address of remote peer
 * @param port Port of remote peer
 * @param cmd Command ID
 * @param data Pointer to data
 * @param data_len Data size
 *
 * @return Return 0 if success; -1 if data length is too lage (more than MAX_PACKET_LEN)
 */
int ksnCoreSendto(ksnCoreClass *kc, char *addr, int port, uint8_t cmd,
                  void *data, size_t data_len) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)kc->ke), MODULE, DEBUG_VV,
                 "send %d bytes data, cmd %u to %s:%d\n", data_len, cmd, addr, port);
    #endif


    int retval = 0;

    if(data_len <= MAX_PACKET_LEN - MAX_DATA_LEN) {

        struct sockaddr_storage remaddr;         // remote address
        socklen_t addrlen = sizeof(remaddr);// length of addresses
        make_addr(addr, port, (__SOCKADDR_ARG) &remaddr, &addrlen);

        // Split large packet
        void **packets;
        int num_subpackets = 0;
        if(cmd != CMD_VPN)
            packets = ksnSplitPacket(kc->kco->ks, cmd, data, data_len, &num_subpackets);

        // Send large packet
        if(num_subpackets) {

            int i;

            // Encrypt and send sub-packets
            for(i = 0; i < num_subpackets; i++) {

                // Create packet
                size_t packet_len;
                size_t data_len = *(uint16_t*)(packets[i]);
                void *packet = ksnCoreCreatePacket(kc, CMD_SPLIT,
                        packets[i] + sizeof(uint16_t),
                        data_len + sizeof(uint16_t)*2,
                        &packet_len);

                // Encrypt and send one spitted sub-packet
                sendto_encrypt(kc, CMD_SPLIT, packet, packet_len);

                // Free memory
                free(packet);
                free(packets[i]);
            }
            free(packets);
        }

        // Send small packet
        else {

            // Create packet
            size_t packet_len;
            void *packet = ksnCoreCreatePacket(kc, cmd, data, data_len,
                    &packet_len);

            // Encrypt and send one not spitted packet
            sendto_encrypt(kc, cmd, packet, packet_len);

            // Free packet
            free(packet);
        }

    }
    else {
        #ifdef DEBUG_KSNET
        ksn_printf(((ksnetEvMgrClass*)kc->ke), MODULE, ERROR_M,
                     "can't send to lage packet %d bytes data, cmd %u to %s:%d\n", data_len, cmd, addr, port);
        #endif
        retval = -1;   // Error: To lage packet
    }

    // Set last host event time
    ksnCoreSetEventTime(kc);

    return retval;
}

typedef struct send_by_type_check_t {
    char *name;
    int num;
    ksnet_arp_data_ext **arp;
} send_by_type_check_t;

int send_by_type_check_cb(ksnetArpClass *ka, char *peer_name,
        ksnet_arp_data_ext *arp,void *pd) {

    send_by_type_check_t *sd = pd;
    if(arp->type && strstr(arp->type, sd->name)) {
        if(!sd->num) sd->arp = malloc(sizeof(ksnet_arp_data *));
        else sd->arp = realloc(sd->arp, sizeof(ksnet_arp_data *) * (sd->num + 1));

        //*(sd->arp + sd->num) = arp;
        sd->arp[sd->num] = arp;

        sd->num++;
    }
    return 0;
}

/**
 * Send brodcast command to peers by type
 *
 * @param kc Pointer to ksnCoreClass
 * @param to Peer name to send to
 * @param cmd Command
 * @param data Commands data
 * @param data_len Commands data length
 * @return Pointer to ksnet_arp_data or NULL if to peer is absent
 */
void teoBroadcastSend(ksnCoreClass *kc, char *to, uint8_t cmd, void *data, size_t data_len) {
    send_by_type_check_t sd = { .name = to, .num = 0 };
    ksnetEvMgrClass* ke = (ksnetEvMgrClass*)(kc->ke);

    ksnetArpGetAll(kc->ka, send_by_type_check_cb, &sd);

    if (!sd.num) {
        return;
    }

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG, "send broadcast message by type \"%s\" \n", to);
    #endif
    
    for(int i=0; i < sd.num; ++i) {
        ksnCoreSendto(kc, sd.arp[i]->data.addr, sd.arp[i]->data.port, cmd, data, data_len);
    }

    free(sd.arp);
}

/**
 * Send command by name to peer
 *
 * @param kc Pointer to ksnCoreClass
 * @param to Peer name to send to
 * @param cmd Command
 * @param data Commands data
 * @param data_len Commands data length
 * @return Pointer to ksnet_arp_data or NULL if to peer is absent
 */
ksnet_arp_data *ksnCoreSendCmdto(ksnCoreClass *kc, char *to, uint8_t cmd,
                                 void *data, size_t data_len) {

    int fd;
    ksnet_arp_data *arp;
    send_by_type_check_t sd = { .name = to, .num = 0 };
    ksnetEvMgrClass* ke = (ksnetEvMgrClass*)(kc->ke);

    // Send to peer in this network
    if((arp = (ksnet_arp_data *)ksnetArpGet(kc->ka, to)) != NULL/* && arp->mode != -1*/) {
        ksnCoreSendto(kc, arp->addr, arp->port, cmd, data, data_len);
    }

    // Send by type in this network
    else if(!ksnetArpGetAll(kc->ka, send_by_type_check_cb, &sd) && sd.num) {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG, "send to peer by type \"%s\" \n", to);
        #endif

        int r = rand() % sd.num;
        ksnCoreSendto(kc, sd.arp[r]->data.addr, sd.arp[r]->data.port, cmd, data, data_len);

        free(sd.arp);
    }

    // Send to peer at other network
    else if(ke->km != NULL &&
        (arp = ksnMultiSendCmdTo(ke->km, to, cmd, data,
                data_len))) {

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG, 
                "send to peer \"%s\" at other network\n", to);
        #endif
    }

    // Send this message to L0 client
    else if(cmd == CMD_L0 &&
            ke->teo_cfg.l0_allow_f &&
            (fd = ksnLNullClientIsConnected(ke->kl, to))) {

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
                "send command to L0 client \"%s\" to r-host\n", to);
        #endif

        ksnLNullSPacket *spacket = data;

        const size_t buf_length = teoLNullBufferSize(
                spacket->client_name_length,
                spacket->data_length
        );

        char *buf = malloc(buf_length);
        teoLNullPacketCreate(buf, buf_length,
                spacket->cmd,
                spacket->payload,
                (uint8_t *)spacket->payload + spacket->client_name_length,
                (size_t)spacket->data_length);

        ssize_t snd = ksnLNullPacketSend(ke->kl, fd, buf, buf_length);
        (void)snd;
        free(buf);
    }

    // Send to r-host
    else {
        ksn_printf(ke, MODULE, DEBUG, "Sending a command %d is not possible, because peer: \"%s\" not found.\n", (int)cmd, to);
    }

    return arp;
}

/**
 * Create ksnet packet
 *
 * @param kc Pointer to ksnCore Class object
 * @param cmd Command ID
 * @param data Pointer to data
 * @param data_len Data length
 * @param [out] packet_len Pointer to packet length
 *
 * @return Pointer to packet. Should be free after use.
 */
//void *ksnCoreCreatePacket(ksnCoreClass *kc, uint8_t cmd, const void *data,
//                          size_t data_len, size_t *packet_len) {
//
//    size_t ptr = 0;
//    *packet_len = kc->name_len + data_len + PACKET_HEADER_ADD_SIZE;
//    void *packet = malloc(*packet_len);
//
//    // Copy packet data
//    *((uint8_t *)packet) = kc->name_len; ptr += sizeof(uint8_t); // Name length
//    memcpy(packet + ptr, kc->name, kc->name_len); ptr += kc->name_len; // Name
//    *((uint8_t *) packet +ptr) = cmd; ptr += sizeof(uint8_t); // Command
//    memcpy(packet + ptr, data, data_len); ptr += data_len; // Data
//
//    return packet;
//}
inline void *ksnCoreCreatePacket(ksnCoreClass *kc, uint8_t cmd, const void *data,
                          size_t data_len, size_t *packet_len) {

    return ksnCoreCreatePacketFrom(kc, cmd, kc->name, kc->name_len, data,
            data_len, packet_len);
}

/**
 * Create ksnet packet with from field from another host (Resend)
 *
 * This function created to resend messages between networks in multi network
 * application
 *
 * @param kc Pointer to ksnCore Class object
 * @param cmd Command ID
 * @param from From name
 * @param from_len From name length
 * @param data Pointer to data
 * @param data_len Data length
 * @param [out] packet_len Pointer to packet length
 *
 * @return Pointer to packet. Should be free after use.
 */
void *ksnCoreCreatePacketFrom(ksnCoreClass *kc, uint8_t cmd,
        char *from, size_t from_len,
        const void *data, size_t data_len, size_t *packet_len) {

    size_t ptr = 0;
    *packet_len = from_len + data_len + PACKET_HEADER_ADD_SIZE;
    void *packet = malloc(*packet_len);

    // Copy packet data
    *((uint8_t *)packet) = from_len; ptr += sizeof(uint8_t); // From name length
    memcpy(packet + ptr, from, from_len); ptr += from_len; // From name
    *((uint8_t *) packet +ptr) = cmd; ptr += sizeof(uint8_t); // Command
    memcpy(packet + ptr, data, data_len); ptr += data_len; // Data

    return packet;
}

/**
 * Parse received data to ksnCoreRecvData structure
 *
 * @param packet Received packet
 * @param packet_len Received packet length
 * @param rd Pointer to ksnCoreRecvData structure
 */
int ksnCoreParsePacket(void *packet, size_t packet_len, ksnCorePacketData *rd) {

    size_t ptr = 0;
    int packed_valid = 0;

    // Raw Packet buffer and length
    rd->raw_data = packet;
    rd->raw_data_len = packet_len;

    rd->from_len = *((uint8_t *)packet); ptr += sizeof(rd->from_len); // From length
    if (rd->from_len &&
        rd->from_len + PACKET_HEADER_ADD_SIZE <= packet_len &&
        *((char *)(packet + ptr + rd->from_len - 1)) == '\0'
    ) {
      rd->from = (char *)(packet + ptr); ptr += rd->from_len; // From pointer
      if(strlen(rd->from) + 1 == rd->from_len) {

        rd->cmd = *((uint8_t *)(packet + ptr)); ptr += sizeof(rd->cmd); // Command ID
        rd->data = packet + ptr; // Data pointer
        rd->data_len = packet_len - ptr; // Data length

        packed_valid = 1;
      }
    }

    return packed_valid;
}

/**
 * Send command connected to peer
 *
 * @param ka Pointer to ksnetArpClass
 * @param child_peer Child peer name
 * @param arp_data Child arp data
 * @param data Pointer to ksnCoreRecvData (included new peer name)
 */
int send_cmd_connected_cb(ksnetArpClass *ka, char *child_peer,
                      ksnet_arp_data *arp_data, void *data) {

    #define rd ((ksnCorePacketData*) data)
    #define new_peer rd->from

    if(strcmp(child_peer, new_peer)) {

        // Send child to new peer
        ksnCommandSendCmdConnect( ((ksnetEvMgrClass*) ka->ke)->kc->kco, new_peer,
                child_peer, arp_data->addr, arp_data->port );

        // Send new peer to child
        ksnCommandSendCmdConnect( ((ksnetEvMgrClass*) ka->ke)->kc->kco, child_peer,
                new_peer, rd->addr, rd->port );
    }

    return 0;

    #undef new_peer
    #undef rd
}

/**
 * Send command disconnect to peer
 *
 * @param ka
 * @param name
 * @param arp_data
 * @param data
 */
int send_cmd_disconnect_cb(ksnetArpClass *ka, char *name,
                            ksnet_arp_data_ext *arp_data, void *data) {

    ksnCoreSendto(((ksnetEvMgrClass*) ka->ke)->kc,
            arp_data->data.addr,
            arp_data->data.port,
            CMD_DISCONNECTED,
            data, data != NULL ? strlen(data)+1 : 0
    );

    return 0;
}

/**
 * Send command to peer to disconnect peer
 *
 * @param ka
 * @param name
 * @param arp_data
 * @param data
 */
int send_cmd_disconnect_peer_cb(ksnetArpClass *ka, char *name,
                            ksnet_arp_data_ext *arp_data, void *data) {

    if(arp_data->data.mode == 2) {
        ksnetArpGetAll(ka, send_cmd_disconnect_cb, name);
    }

    return 0;
}

/**
 * Teonet host fd read event callback
 *
 * param loop
 * @param w
 * @param revents
 */
void host_cb(EV_P_ ev_io *w, int revents) {

    ksnCoreClass *kc = w->data;             // ksnCore Class object
    ksnetEvMgrClass *ke = kc->ke;           // ksnetEvMgr Class object

    struct sockaddr_storage remaddr;             // remote address
    socklen_t addrlen = sizeof(remaddr);    // length of addresses
    unsigned char buf[KSN_BUFFER_DB_SIZE];  // Message buffer
    size_t recvlen;                         // # bytes received

    // Receive data
    recvlen = ksn_recvfrom(ke->kc->ku,
        revents == EV_NONE ? 0 : kc->fd, (char*)buf, KSN_BUFFER_DB_SIZE,
        0, (struct sockaddr *)&remaddr, &addrlen);

    // Process package
    ksnCoreProcessPacket(kc, buf, recvlen, (__SOCKADDR_ARG) &remaddr);

    // Set last host event time
    ksnCoreSetEventTime(kc);
}

typedef struct peer_type_req {
    ksnCoreClass *kc;
    char *addr;
    char *from;
    int port;
} peer_type_req_t;

void peer_type_cb(uint32_t id, int type, void *data) {
    peer_type_req_t *type_req = data;
    if (!type) {//timeout // TODO: rename type
        ksnet_arp_data_ext *arp_cque = ksnetArpGet(type_req->kc->ka, type_req->from);
        if (arp_cque) {
            ksnCoreSendto(type_req->kc, type_req->addr, type_req->port, CMD_HOST_INFO, NULL, 0);
            ksnetEvMgrClass *ke = type_req->kc->ke;

            ksnCQueData *cq = ksnCQueAdd(ke->kq, peer_type_cb, 1, type_req);
            arp_cque->cque_id_peer_type = cq->id;
            ksnetArpAdd(type_req->kc->ka, type_req->from, arp_cque);
        }
    } else {//exec
        free(type_req->addr);
        free(type_req->from);
        free(type_req);
    }
}

/**
 * Check new peer
 *
 * @param kc
 * @param rd
 * @return
 */
void ksnCoreCheckNewPeer(ksnCoreClass *kc, ksnCorePacketData *rd) {

    ksnetEvMgrClass *ke = kc->ke; // ksnetEvMgr Class object

    // Check new peer connected
    if((rd->arp = ksnetArpGet(kc->ka, rd->from)) == NULL) {

        ksnet_arp_data_ext arp;
        rd->arp = &arp;

        // The "mode" variable (0 by default) sets to 1 when this host connected
        // to r-host 
        //
        // The "connect_r" variable (0 by default) sets to 1 when this host is 
        // r-host and other peer send connect to r-host command
        int mode = 0;
        int connect_r = rd->cmd == CMD_CONNECT_R ? 1 : 0;

        // Check that this host connected to r-host
        if(!ke->teo_cfg.r_host_name[0] && ke->teo_cfg.r_port == rd->port &&
           ( 
             ((rd->cmd == CMD_NONE || rd->cmd == CMD_HOST_INFO) && rd->data_len == 2) ||
             !strcmp(ke->teo_cfg.r_host_addr, rd->addr)
           )) {

            #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, DEBUG, "connected to r-host: %s (%s:%d)\n",
                    rd->from, rd->addr, rd->port);
            #endif

            strncpy(ke->teo_cfg.r_host_name, rd->from,
                    sizeof(ke->teo_cfg.r_host_name)-1);
            strncpy(ke->teo_cfg.r_host_addr, rd->addr, sizeof(ke->teo_cfg.r_host_addr) - 1);
            mode = 1;
        }

        // Add peer to ARP Table
        memset(rd->arp, 0, sizeof(*rd->arp));
        strncpy(rd->arp->data.addr, rd->addr, sizeof(rd->arp->data.addr)-1);
        rd->arp->data.connected_time = ksnetEvMgrGetTime(ke);
        rd->arp->data.port = rd->port;
        rd->arp->data.mode = mode;

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
                "new peer %s (%s:%d) connected\n",
                rd->from, rd->addr, rd->port);
        #endif

        // Request host info
        ksnCoreSendto(ke->kc, rd->addr, rd->port, CMD_HOST_INFO, "\0", 1 + connect_r);
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV, "send CMD_HOST_INFO = %u command to (%s:%d)\n",
                CMD_HOST_INFO, rd->addr, rd->port);
        #endif
        peer_type_req_t *type_request = malloc(sizeof(peer_type_req_t));
        type_request->kc = ke->kc;
        type_request->addr = strdup(rd->addr);
        type_request->from = strdup(rd->from);
        type_request->port = rd->port;
        ksnCQueData *cq = ksnCQueAdd(ke->kq, peer_type_cb, 1, type_request);
        rd->arp->cque_id_peer_type = cq->id;

        if(rd->cmd == CMD_CONNECT_R) {
            connect_r_packet_t *packet = rd->data;
            rd->arp->type = strdup(packet->type);
        }

        ksnetArpAdd(kc->ka, rd->from, rd->arp);
        rd->arp = ksnetArpGet(kc->ka, rd->from);

        // Send child address to r-host (useful when connect one r-host to another)
        if(mode /*&& ke->is_rhost*/) {
            ksnCorePacketData rd_;
            rd_.from = rd->from;
            rd_.addr = rd->addr;
            rd_.port = rd->port;
            #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, DEBUG, "resend child to r-host: %s (%s:%d)\n",
                    rd_.from, rd_.addr, rd_.port);
            #endif
            ksnetArpGetAll(ke->kc->ka, send_cmd_connect_cb_b, &rd_);
            ksnetArpGetAll(ke->kc->ka, send_cmd_connect_cb, &rd_);
        }
    } 

    // If existed peer Got CMD_NONE (answer for connect to r-host command) 
    // than set it as r-host
    else if(rd->cmd == CMD_NONE && rd->data_len == 2) {

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG, "already connect to r-host peer (set as connected to r-host), got from: %s (%s:%d)\n",
                rd->from, rd->addr, rd->port);
        #endif

        // ke->is_rhost = true;
        strncpy(ke->teo_cfg.r_host_name, rd->from, sizeof(ke->teo_cfg.r_host_name)-1);
        rd->arp->data.mode = 1;
    }
}

/**
 * Process teonet packet
 *
 * @param vkc Pointer to ksnCoreClass
 * @param buf Buffer with packet
 * @param recvlen Packet length
 * @param remaddr Address
 */
void ksnCoreProcessPacket (void *vkc, void *buf, size_t recvlen, __SOCKADDR_ARG remaddr) {

    ksnCoreClass *kc = vkc; // ksnCoreClass Class object
    ksnetEvMgrClass *ke = kc->ke; // ksnetEvMgr Class object

    if (ksnetEvMgrStatus(ke) == kEventMgrStopped) {
        return;
    }

    // Data received
    if(recvlen > 0) {
        addr_port_t *ap_obj = wrap_inet_ntop(remaddr);
        char *addr = strdup(ap_obj->addr); // IP to string
        int port = ap_obj->port;
        addr_port_free(ap_obj);

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
                "got %d bytes from %s:%d\n", recvlen, addr, port);
        #endif

        void *data; // Decrypted packet data
        size_t data_len; // Decrypted packet data length

        // Decrypt package
        int encrypted = 0;
        #if KSNET_CRYPT        
        if(ke->teo_cfg.crypt_f && ksnCheckEncrypted(buf, recvlen)) {
            encrypted = 1;
            data = ksnDecryptPackage(kc->kcr, buf, recvlen, &data_len);
        } else { // Use packet without decryption
        #endif
            data = buf;
            data_len = recvlen;
        #if KSNET_CRYPT
        }
        #endif

        /****************************************************/
        /* KSNet core packet structure                      */
        /****************************************************/
        /* from length *  from         * command *   data   */
        /****************************************************/
        /* 1 byte      *  from length  * 1 byte  *  N byte  */
        /****************************************************/
        //
        // from length - uint8_t: length of from (with 0 at the end)
        // from - char[]: from name
        // data - uint8_t[]: data
        // data_length = packet_length - 1 - from_length
        //

        // Parse ksnet packet and Fill ksnet receive data structure
        ksnCorePacketData rd;
        memset(&rd, 0, sizeof(rd));
        int event = EV_K_RECEIVED;
        int command_processed = 0;

        // Remote peer address and peer
        rd.addr = addr; // IP to string
        rd.port = port; // Port to integer

        // Parse packet and check if it valid
        //    
        // In this place we check two type of packets one from teonet another 
        // from teonet client.
        // 
        // Teonet packets are encrypte (but only one packet with cmd == CMD_L0 
        // may be not encrypted) & ksnCoreParsePacket return true.
        //
        // Teocli packet is not encrypted and ksnCoreParsePacket return false.
        // ksnCoreParsePacket sometimes may return false positive. So we use 
        // additional precautions in this if.
        //
        // CMD_L0 => #70 Command from L0 Client    
        if( ksnCoreParsePacket(data, data_len, &rd) && ( encrypted || rd.cmd == CMD_L0 ) ) {
            // Check ARP Table and add peer if not present            
            #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, DEBUG_VV,
                "recieve command = %d, from %s (%s:%d). (%d byte)\n",
                rd.cmd, rd.from, rd.addr, rd.port, rd.data_len);
            #endif

            // Check new peer connected
            ksnCoreCheckNewPeer(kc, &rd);

            // Set last activity time
            rd.arp->data.last_activity = ksnetEvMgrGetTime(ke);

            // Check & process command
            command_processed = ksnCommandCheck(kc->kco, &rd);
        } else { 
            
            rd.from = "";
            rd.from_len = 1;
            rd.data = data;
            rd.data_len = data_len;
            // Check TR-UDP L0 packet and process it if valid
            if(!ke->kl || !ksnLNulltrudpCheckPaket(ke->kl, &rd)) {
                event = EV_K_RECEIVED_WRONG;
                #ifdef DEBUG_KSNET
                ksn_printf(ke, MODULE, DEBUG_VV,
                    "WRONG RECEIVED! cmd = %d, from: %s %s:%d\n", rd.cmd, rd.from, rd.addr, rd.port);
                #endif
            }

            command_processed = 1;
        }

        // Send event to User level
        if(!command_processed && ke->event_cb) {
            ke->event_cb(ke, event, (void*)&rd, sizeof(rd), NULL);
        }

        free(addr);
    }
}
