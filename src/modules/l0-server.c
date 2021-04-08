/**
 * File:   l0-server.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * L0 Server module
 *
 * Created on October 8, 2015, 1:38 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "ev_mgr.h"
#include "l0-server.h"
#include "utils/rlutil.h"
#include "jsmn.h"

#include "teonet_l0_client_crypt.h"

#include <openssl/md5.h>

#define MODULE _ANSI_LIGHTCYAN "l0_server" _ANSI_NONE
#define TEO_AUTH "teo-auth"
#define WG001 "wg001-"
#define WG001_NEW "wg001-new-"

// Local functions
static int ksnLNullStart(ksnLNullClass *kl);
static void ksnLNullStop(ksnLNullClass *kl);
static ksnet_arp_data *ksnLNullSendFromL0(ksnLNullClass *kl, teoLNullCPacket *packet,
        char *cname, size_t cname_length);
static ksnLNullData* ksnLNullClientRegister(ksnLNullClass *kl, int fd, const char *remote_addr, int remote_port);
static ssize_t ksnLNullSend(ksnLNullClass *kl, int fd, uint8_t cmd, void* data, size_t data_length);
static int ksnLNullSendBroadcast(ksnLNullClass *kl, uint8_t cmd, void* data, size_t data_length);
static bool ksnLNullClientAuthCheck(ksnLNullClass *kl, ksnLNullData *kld, int fd, teoLNullCPacket *packet);
static bool sendKEXResponse(ksnLNullClass *kl, ksnLNullData *kld, int fd);
static bool processKeyExchange(ksnLNullClass *kl, ksnLNullData *kld, int fd,
        KeyExchangePayload_Common *kex, size_t kex_length);


// Other modules not declared functions
void *ksnCoreCreatePacket(ksnCoreClass *kc, uint8_t cmd, const void *data,
        size_t data_len, size_t *packet_len);

// External constants
extern const char *localhost;

/**
 * Pointer to ksnetEvMgrClass
 *
 */
#define kev ((ksnetEvMgrClass*)kl->ke)
#define L0_VERSION 0 ///< L0 Server version

void teoLNullPacketCheckMiscrypted(ksnLNullClass *kl, ksnLNullData *kld,
                                   teoLNullCPacket *packet) {
    if (packet->reserved_2 == 0 || packet->data_length == 0) {
        return;
    }

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    uint8_t *p_data = teoLNullPacketGetPayload(packet);
    uint8_t data_check = get_byte_checksum(p_data, packet->data_length);
    if (data_check == 0) { data_check++; }
    if (data_check == packet->reserved_2) {
        return;
    }

    char hexdump[92];
    dump_bytes(hexdump, sizeof(hexdump), p_data, packet->data_length);

    ksn_printf(ke, MODULE, DEBUG,
               "FAILED " _ANSI_RED "POST DECRYPTION " _ANSI_YELLOW
               "CHECK from %s:%d" _ANSI_LIGHTCYAN
               " cmd %u, peer '%s', data_length %d, checksum %d, "
               "header_checksum %d, data checksum in packet " _ANSI_RED
               "%d != %d" _ANSI_LIGHTCYAN " as calculated, data: " _ANSI_NONE
               " %s\n",
               kld->t_addr, kld->t_port, (uint32_t)packet->cmd,
               packet->peer_name, (uint32_t)packet->data_length,
               (uint32_t)packet->checksum, (uint32_t)packet->header_checksum,
               (uint32_t)packet->reserved_2, data_check, hexdump);
}

/**
 * Initialize ksnLNull module [class](@ref ksnLNullClass)
 *
 * @param ke Pointer to ksnetEvMgrClass
 *
 * @return Pointer to created ksnLNullClass or NULL if error occurred
 */
ksnLNullClass *ksnLNullInit(void *ke) {

    ksnLNullClass *kl = NULL;

    if(((ksnetEvMgrClass*)ke)->teo_cfg.l0_allow_f) {

        kl = malloc(sizeof(ksnLNullClass));
        if(kl != NULL)  {
            kl->ke = ke; // Pointer event manager class
            kl->map = pblMapNewHashMap(); // Create a new hash map
            kl->map_n = pblMapNewHashMap(); // Create a new hash map
            memset(&kl->stat, 0, sizeof(kl->stat)); // Clear statistic data
            kl->fd_trudp = MAX_FD_NUMBER;
            ksnLNullStart(kl); // Start L0 Server
        }
    }

    return kl;
}

/**
 * Generate next fake fd number for trudp client connection [class](@ref ksnLNullClass)
 *
 * @param kl Pointer to ksnLNullClass
 * 
 * @return sequentially ingremented integer out of range of what real fs's can be
 */
static int ksnLNullGetNextFakeFd(ksnLNullClass *kl) {
    // Fixup if had overflow earlier
    if (kl->fd_trudp < MAX_FD_NUMBER) {
        kl->fd_trudp = MAX_FD_NUMBER;
    }
    return kl->fd_trudp++;
}

/**
 * Get client connection
 *
 * @param kl Pointer to ksnLNullClass
 * @param fd connection fd
 *
 * @return Client or NULL if not connected
 */
static ksnLNullData* ksnLNullGetClientConnection(ksnLNullClass *kl, int fd) {
    size_t valueLength;
    return pblMapGet(kl->map, &fd, sizeof(fd), &valueLength);
}

/**
 * Destroy ksnLNull module [class](@ref ksnLNullClass)
 *
 * @param kl Pointer to ksnLNullClass
 */
void ksnLNullDestroy(ksnLNullClass *kl) {

    if(kl != NULL) {
        ksnLNullStop(kl);
//        teoSScrDestroy(kl->sscr);
        free(kl->map_n);
        free(kl->map);
        free(kl);
    }
}

/**
 * L0 Server client callback
 *
 * Get packet from L0 Server client connection and resend it to teonet
 *
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 *
 */
static void cmd_l0_read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {

    size_t buffer_len = KSN_BUFFER_DB_SIZE; // Buffer length
    ksnLNullClass *kl = w->data; // Pointer to ksnLNullClass
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);
    char buffer[buffer_len]; // Buffer

    // Read TCP data
    ssize_t received = read(w->fd, buffer, buffer_len);
    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV,
            "Got something from fd %d w->events = %d, received = %d ...\n",
            w->fd, w->events, (int)received);
    #endif

    // Disconnect client:
    // Close TCP connections and remove data from l0 clients map
    if(!received) {

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
            "Connection closed. Stop listening fd %d ...\n",
            w->fd
        );
        #endif

        ksnLNullClientDisconnect(kl, w->fd, 1);
    } else if (received < 0) { // \TODO: Process reading error
        //        if( errno == EINTR ) {
        //            // OK, just skip it
        //        }
        //        #ifdef DEBUG_KSNET
        //        ksnet_printf(
        //            &ke->teo_cfg, DEBUG,
        //            "%sl0 Server:%s "
        //            "Read error ...%s\n",
        //            ANSI_LIGHTCYAN, ANSI_RED, ANSI_NONE
        //        );
        //        #endif
    } else { // Success read. Process package and resend it to teonet
        ksnLNullData* kld = ksnLNullGetClientConnection(kl, w->fd);
        if(kld != NULL) {

            // Set last time received
            kld->last_time = ksnetEvMgrGetTime(kl->ke);

            // Add received data to the read buffer
            if(received > kld->read_buffer_size - kld->read_buffer_ptr) {

                // Increase read buffer size
                kld->read_buffer_size += buffer_len; //received;
                if(kld->read_buffer != NULL)
                    kld->read_buffer = realloc(kld->read_buffer,
                            kld->read_buffer_size);
                else
                    kld->read_buffer = malloc(kld->read_buffer_size);

                #ifdef DEBUG_KSNET
                ksn_printf(ke, MODULE, DEBUG_VV,
                    "%s" "increase read buffer to new size: %d bytes ...%s\n",
                    ANSI_DARKGREY, kld->read_buffer_size,
                    ANSI_NONE);
                #endif
            }
            memmove((uint8_t *)kld->read_buffer + kld->read_buffer_ptr, buffer, received);
            kld->read_buffer_ptr += received;

            teoLNullCPacket *packet = (teoLNullCPacket *)kld->read_buffer;
            size_t len, ptr = 0;

            // \todo Check packet

            // Process read buffer
            while(kld->read_buffer_ptr - ptr >= (len = sizeof(teoLNullCPacket) +
                    packet->peer_name_length + packet->data_length)) {

                // Check checksum
                uint8_t *packet_header = (uint8_t *)packet;
                uint8_t *packet_body = (uint8_t *)packet->peer_name;
                if(packet->header_checksum == get_byte_checksum(packet_header,
                    sizeof(teoLNullCPacket) - sizeof(packet->header_checksum)) &&
                   packet->checksum == get_byte_checksum(packet_body,
                        packet->peer_name_length + packet->data_length)) {

                    if (teoLNullPacketDecrypt(kld->server_crypt, packet)) {
                        teoLNullPacketCheckMiscrypted(kl, kld, packet);

                        // Check initialize packet:
                        // cmd = 0, to_length = 1, data_length = 1 + data_len,
                        // data = client_name
                        //
                        if(packet->cmd == 0 && packet->peer_name_length == 1 &&
                            !packet->peer_name[0] && packet->data_length) {

                            uint8_t *data = teoLNullPacketGetPayload(packet);
                            KeyExchangePayload_Common *kex =
                                teoLNullKEXGetFromPayload(data,
                                                          packet->data_length);
                            if (kex) {
                                // received client keys
                                // must be first time or exactly the same as previous
                                if (processKeyExchange(kl, kld, w->fd, kex,
                                                       packet->data_length)) {
                                    #ifdef DEBUG_KSNET
                                    ksn_printf(ke, MODULE, DEBUG_VV,
                                               "Encription established for fd %d ...\n",
                                               w->fd);
                                    #endif
                                } else {
                                    #ifdef DEBUG_KSNET
                                    ksn_printf(ke, MODULE, DEBUG_VV,
                                               "Key exchange failed. Stop listening fd %d ...\n",
                                               w->fd);
                                    #endif

                                    ksnLNullClientDisconnect(kl, w->fd, 1);
                                    return;
                                }
                            } else {
                                // process login here
                                size_t expected_len = packet->data_length - 1;
                                if (expected_len > 0 &&
                                    data[expected_len] == 0 &&
                                    strlen((const char *)data) == expected_len) {
                                    // Set temporary name (it will be changed after TEO_AUTH answer)
                                    ksnLNullClientAuthCheck(kl, kld, w->fd, packet);
                                } else {
                                    ksn_printf(ke, MODULE, ERROR_M,
                                        "Invalid login packet from %s:%d\n",
                                        kld->t_addr, kld->t_port);
                                }
                            }
                        }

                        // Resend data to teonet or drop wrong packet
                        else {
                            // Resend data to teonet
                            if(kld->name) {
                                ksnLNullSendFromL0(kl, packet, kld->name, kld->name_length);
                            }
                            // Drop wrong packet
                            else {
                                #ifdef DEBUG_KSNET
                                ksn_printf(ke, MODULE, DEBUG,
                                    "%s" "got valid packet from fd: %d before login command; packet ignored ...%s\n",
                                    ANSI_RED, w->fd, ANSI_NONE);
                                #endif
                            }
                        }
                    } else {
                        #ifdef DEBUG_KSNET
                        ksn_printf(ke, MODULE, DEBUG,
                                   _ANSI_RED
                                   "FAILED decrypt packet from "
                                   "fd: %d; packet ignored ..." _ANSI_NONE "\n",
                                   w->fd);
                        #endif
                    }
                }

                // Wrong checksum - drop this packet
                else {
                    #ifdef DEBUG_KSNET
                    ksn_printf(ke, MODULE, DEBUG,
                        "%s" "got wrong packet %d bytes length, from fd: %d; packet dropped ...%s\n",
                        ANSI_RED, len, w->fd, ANSI_NONE);
                    #endif
                }

                ptr += len;
                packet = (void *)((uint8_t*)packet + len);
            }

            // Check end of buffer
            if(kld->read_buffer_ptr - ptr > 0) {

                #ifdef DEBUG_KSNET
                ksn_printf(ke, MODULE, DEBUG_VV,
                    "%s" "wait next part of packet, now it has %d bytes ...%s\n",
                    ANSI_DARKGREY, kld->read_buffer_ptr - ptr,
                    ANSI_NONE);
                #endif

                kld->read_buffer_ptr = kld->read_buffer_ptr - ptr;
                memmove(kld->read_buffer, (uint8_t *)kld->read_buffer + ptr,
                        kld->read_buffer_ptr);
            }
            else kld->read_buffer_ptr = 0;
        }
    }
}

/**
 * Get L0 server statistic
 *
 * @param kl Pointer to ksnLNullClass
 * @return Pointer to ksnLNullSStat or NULL if L0 server has not started
 */
ksnLNullSStat *ksnLNullStat(ksnLNullClass *kl) {

    ksnLNullSStat *stat = NULL;

    if(kl != NULL) {

        stat = &kl->stat;
    }

    return stat;
}

/**
 * Extend L0 log
 */ 
static int extendedLog(ksnLNullClass *kl) {
    int log_level = DEBUG_VV;
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);
    if(ke->teo_cfg.l0_allow_f && ke->teo_cfg.extended_l0_log_f) {
        log_level = DEBUG;
    }
    return log_level;
}

/**
 * Send data received from L0 client to teonet peer
 *
 * @param kl Pointer to ksnLNullClass
 * @param packet L0 packet received from L0 client
 * @param cname L0 client name (include trailing zero)
 * @param cname_length Length of the L0 client name
 * @return Pointer to ksnet_arp_data or NULL if to peer is absent
 */
static ksnet_arp_data *ksnLNullSendFromL0(ksnLNullClass *kl, teoLNullCPacket *packet,
        char *cname, size_t cname_length) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);
    size_t out_data_len = sizeof(ksnLNullSPacket) + cname_length +
            packet->data_length;
    char *out_data = malloc(out_data_len);
    memset(out_data, 0, out_data_len);
    ksnLNullSPacket *spacket = (ksnLNullSPacket*) out_data;

    // Create teonet L0 packet
    spacket->cmd = packet->cmd;
    spacket->client_name_length = cname_length;
    memcpy(spacket->payload, cname, cname_length);
    spacket->data_length = packet->data_length;
    memcpy(spacket->payload + spacket->client_name_length,
        packet->peer_name + packet->peer_name_length, spacket->data_length);

    // Send teonet L0 packet
    ksnet_arp_data *arp_data = NULL;
    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, extendedLog(kl),
        "send packet to peer \"%s\" from L0 client \"%s\" ...\n",
        packet->peer_name, spacket->payload);
    #endif

    // Send to peer
    if(strlen((char*)packet->peer_name) && strcmp((char*)packet->peer_name, ksnetEvMgrGetHostName(ke))) {        
        arp_data = ksnCoreSendCmdto(ke->kc, packet->peer_name, CMD_L0,
                spacket, out_data_len);
    }
    // Send to this host
    else {
        // Create packet
        size_t pkg_len;
        void *pkg = ksnCoreCreatePacket(kev->kc, CMD_L0, spacket, out_data_len,
                    &pkg_len);
        struct sockaddr_storage addr;             // address structure
        socklen_t addrlen = sizeof(addr); // length of addresses

        int fd = ksnLNullClientIsConnected(kl, spacket->from);
        if (fd == 0) {
            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, extendedLog(kl),
                "client \"%s\" is not connected\n",
                spacket->from);
            #endif
        } else {
            ksnLNullData *kld = ksnLNullGetClientConnection(kl, fd);
            if (kld == NULL) {
                #ifdef DEBUG_KSNET
                ksn_printf(kev, MODULE, extendedLog(kl),
                    "client \"%s\", fd: %d connection data not found\n",
                    spacket->from, fd);
                #endif
            } else if(!make_addr(kld->t_addr, kld->t_port, (__SOCKADDR_ARG) &addr,
                    &addrlen)) {
                #ifdef DEBUG_KSNET
                ksn_printf(kev, MODULE, extendedLog(kl),
                    "repacked packet from client \"%s\" to CMD_L0 from %s:%d\n",
                    spacket->from, kld->t_addr, kld->t_port);
                #endif
                ksnCoreProcessPacket(kev->kc, pkg, pkg_len, (__SOCKADDR_ARG) &addr);
                arp_data = (ksnet_arp_data *)ksnetArpGet(kev->kc->ka, (char*)packet->peer_name);
            }
        }

        free(pkg);
    }

    // Send packet to peer statistic
    kl->stat.packets_to_peer++;

    free(out_data);

    return arp_data;
}

/**
 * Send data to L0 client. Usually it is an answer to request from L0 client
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param addr IP address of remote peer
 * @param port Port of remote peer
 * @param cname L0 client name (include trailing zero)
 * @param cname_length Length of the L0 client name
 * @param cmd Command
 * @param data Data
 * @param data_len Data length
 *
 * @return Return 0 if success; -1 if data length is too lage (more than 32319)
 */
int ksnLNullSendToL0(void *ke, char *addr, int port, char *cname,
        size_t cname_length, uint8_t cmd, void *data, size_t data_len) {

    int fd = 0;
    int rv = -1;

    ksnLNullClass *kl = ((ksnetEvMgrClass*)ke)->kl;
    if (((ksnetEvMgrClass*)ke)->ksn_cfg.l0_allow_f) {
        fd = ksnLNullClientIsConnected(kl, cname);
    }

    if (!fd) {
        //NOTE: client is connected to some other l0 service
        // Create  Server L0 packet
        size_t out_data_len = sizeof(ksnLNullSPacket) + cname_length + data_len;
        //char out_data[out_data_len]; // Buffer
        char *out_data = malloc(out_data_len);
        memset(out_data, 0, out_data_len);
        ksnLNullSPacket *spacket = (ksnLNullSPacket*)out_data;

        // Create teonet L0 packet
        spacket->cmd = cmd;
        spacket->from_length = cname_length;
        memcpy(spacket->from, cname, cname_length);
        spacket->data_length = data_len;
        memcpy(spacket->from + spacket->from_length, data, data_len);

        // Send command to client of L0 server
        #ifdef DEBUG_KSNET
        ksn_printf(((ksnetEvMgrClass*)ke), MODULE, DEBUG_VV,
            "send command to L0 server for client \"%s\" ...\n",
            cname);
        #endif
        rv = ksnCoreSendto(((ksnetEvMgrClass*)ke)->kc, addr, port, CMD_L0_TO,
            out_data, out_data_len);

        free(out_data);
    } else {
        // Create Client L0 packet
        const char* host_name = ksnetEvMgrGetHostName((ksnetEvMgrClass*)ke);
        size_t out_data_len = sizeof(teoLNullCPacket) + strlen(host_name) + 1 + data_len;
        char *out_data = malloc(out_data_len);
        memset(out_data, 0, out_data_len);
        size_t packet_length = teoLNullPacketCreate(out_data, out_data_len,
                cmd, host_name, data, data_len);

        // Send command to L0 client
        if((rv = ksnLNullPacketSend(kl, fd, out_data, packet_length)) >= 0);

        #ifdef DEBUG_KSNET
        teoLNullCPacket *packet = (teoLNullCPacket *)out_data;
        //void *packet_data = packet->peer_name + packet->peer_name_length;
        ksn_printf((ksnetEvMgrClass*)ke, MODULE, DEBUG_VV,
            "send %d bytes to \"%s\" L0 client: %d bytes data, "
            "from peer \"%s\"\n",
            (int)rv, cname,
            packet->data_length, packet->peer_name);
        #endif
        free(out_data);
        if (rv > 0) {
            rv = 0;
        }
    }

    return rv;
}

/**
 * Send echo to L0 client.
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param addr IP address of remote peer
 * @param port Port of remote peer
 * @param cname L0 client name (include trailing zero)
 * @param cname_length Length of the L0 client name
 * @param data Data
 * @param data_len Data length
 *
 * @return Return 0 if success; -1 if data length is too lage (more than 32319)
 */
int ksnLNullSendEchoToL0(void *ke, char *addr, int port, char *cname,
        size_t cname_length, void *data, size_t data_len) {

    size_t data_e_length;
    void *data_e = ksnCommandEchoBuffer(((ksnetEvMgrClass*)ke)->kc->kco, data,
            data_len, &data_e_length);

    int retval = ksnLNullSendToL0(ke, addr, port, cname, cname_length, CMD_ECHO,
            data_e, data_e_length);
    free(data_e);
    return retval;
}

int ksnLNullSendEchoToL0A(void *ke, char *addr, int port, char *cname,
        size_t cname_length, void *data, size_t data_len) {

    size_t data_e_length;
    void *data_e = ksnCommandEchoBuffer(((ksnetEvMgrClass*)ke)->kc->kco, data,
            data_len, &data_e_length);

    ksnCorePacketData rd;
    rd.addr = addr;
    rd.port = port;
    rd.from = cname;
    rd.from_len = cname_length;
    rd.l0_f = 1;
    sendCmdAnswerToBinaryA(ke, &rd, CMD_ECHO, data_e, data_e_length);

    free(data_e);
    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"


/**
 * Register new client in clients map
 *
 * @param kl Pointer to ksnLNullClass
 * @param fd TCP client connection file descriptor
 * @param remote_addr remote address string
 * @param remote_port remote port
 * 
 * @return client, pointer to ksnLNullData
 */
static ksnLNullData* ksnLNullClientRegister(ksnLNullClass *kl, int fd, const char *remote_addr, int remote_port) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    ksnLNullData data;
    data.name = NULL;
    data.name_length = 0;
    data.server_crypt = NULL;
    data.read_buffer = NULL;
    data.read_buffer_ptr = 0;
    data.read_buffer_size = 0;
    data.t_addr = remote_addr ? strdup(remote_addr) : NULL;
    data.t_port = remote_port;
    data.t_channel = 0;
    data.last_time = ksnetEvMgrGetTime(kl->ke);
    pblMapAdd(kl->map, &fd, sizeof(fd), &data, sizeof(ksnLNullData));

    ksnLNullData* kld = ksnLNullGetClientConnection(kl, fd);
    // #ifdef DEBUG_KSNET
    if (kld == NULL) {
        ksn_printf(ke, MODULE, ERROR_M,
                   "Failed L0 client registration with fd %d from %s:%d\n",
                   fd, remote_addr, remote_port);
        return NULL;
    }
    // #endif

    // L0 statistic - client connected
    kl->stat.clients++;

    return kld;
}


void _send_subscribe_event_disconnected(ksnetEvMgrClass *ke, char *payload,
        size_t payload_length) {
    teoSScrSend(ke->kc->kco->ksscr, EV_K_L0_DISCONNECTED, (void *)payload, payload_length, 0);
}

/**
 * Send Connected event to all subscribers
 *
 */
void _send_subscribe_event_connected(ksnetEvMgrClass *ke, char *payload,
        size_t payload_length) {

    teoSScrSend(ke->kc->kco->ksscr, EV_K_L0_CONNECTED, (void *)payload, payload_length, 0);

    ke->kl->stat.visits++;
}

/**
 * Send "new visit" event to all subscribers
 */
void _send_subscribe_event_newvisit(ksnetEvMgrClass *ke, const char *payload,
        size_t payload_length) {

    size_t vd_length = sizeof(ksnLNullSVisitsData) + payload_length;
    ksnLNullSVisitsData *vd = malloc(vd_length);
    vd->visits = ke->kl->stat.visits;
    memcpy(vd->client, payload, payload_length);
    teoSScrSend(ke->kc->kco->ksscr, EV_K_L0_NEW_VISIT, (void *)vd, vd_length, 0);
    free(vd);
}

/**
 * \TODO: move this function to utils
 */
static bool json_eq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return true;
    }
    return false;
}

typedef struct string_view
{
    char *data;
    size_t len;
} string_view;

static void stringViewReset(string_view *s) {
    if (s == NULL) { return; }

    s->data = NULL;
    s->len = 0;
}


static bool parseAuthPayload(ksnLNullClass *kl, uint8_t *payload, string_view *name, string_view *sign, string_view *auth_data_valid_until) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);
    if (name == NULL || sign == NULL || auth_data_valid_until == NULL) {
        ksn_printf(ke, MODULE, DEBUG, "Missing output params: name=%p, sign=%p, valid_ts=%p\n", name, sign, auth_data_valid_until);
        return false;
    }

    char *jsondata = (char *)payload;
    jsmn_parser p;
    jsmntok_t tokens[128];

    jsmn_init(&p);
    int token_num = jsmn_parse(&p, jsondata, strlen(jsondata), tokens, sizeof(tokens)/sizeof(tokens[0]));
    if (token_num < 0) {
        printf("Failed to parse JSON: %d\n", token_num);
        return false;
    }

    if (token_num < 1 || tokens[0].type != JSMN_OBJECT) {
        printf("Object expected\n");
        return false;
    }

    stringViewReset(name);
    stringViewReset(sign);
    stringViewReset(auth_data_valid_until);

    for (int i = 1; i < token_num; i++) {
        if (json_eq(jsondata, &tokens[i], "userId")) {
            printf("ID: %.*s\n", tokens[i+1].end - tokens[i+1].start, jsondata + tokens[i+1].start);
            name->data = (char*)jsondata + tokens[i+1].start;
            name->len = tokens[i+1].end - tokens[i+1].start;
            i++;
        } else if (json_eq(jsondata, &tokens[i], "sign")) {
            printf("SIGN: %.*s\n", tokens[i+1].end - tokens[i+1].start, jsondata + tokens[i+1].start);
            sign->data = (char*)jsondata + tokens[i+1].start;
            sign->len = tokens[i+1].end - tokens[i+1].start;
            i++;
        } else if (json_eq(jsondata, &tokens[i], "timestamp")) {
            printf("TS: %.*s\n", tokens[i+1].end - tokens[i+1].start,
                    jsondata + tokens[i+1].start);
            auth_data_valid_until->data = (char*)jsondata + tokens[i+1].start;
            auth_data_valid_until->len = tokens[i+1].end - tokens[i+1].start;
            i++;
        }
    }

    return auth_data_valid_until->data != NULL && sign->data != NULL && name->data != NULL;
}

static bool checkAuthData(ksnLNullClass *kl, string_view *name, string_view *auth_data_valid_until, string_view *sign, const char *secret) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    if (sign->len != MD5_DIGEST_LENGTH*2) {
        ksn_printf(ke, MODULE, DEBUG, "Invalid signature length:  %.*s\n",
            sign->len, sign->data);
        return false;
    }

    char current_time_str[64];
    //TODO: not sure that this cast to int is valid
    int current_time = ksnetEvMgrGetTime(((ksnetEvMgrClass*)kl->ke));
    snprintf(current_time_str, sizeof(current_time_str), "%d", current_time);

    int current_time_str_len = strlen(current_time_str);
    if ((current_time_str_len > auth_data_valid_until->len) ||
            (current_time_str_len == auth_data_valid_until->len &&
            strncmp(auth_data_valid_until->data, current_time_str, auth_data_valid_until->len) < 0)) {
        ksn_printf(ke, MODULE, DEBUG, "timestamp %.*s outdated, current time: %d\n",
            auth_data_valid_until->len, auth_data_valid_until->data, current_time);
        return false;
    }

    unsigned char digarray[MD5_DIGEST_LENGTH];
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, name->data, name->len);
    MD5_Update(&ctx, secret, strlen(secret));
    MD5_Update(&ctx, auth_data_valid_until->data, auth_data_valid_until->len);
    MD5_Final(digarray, &ctx);

    char md5str[MD5_DIGEST_LENGTH*2 + 1];
    for(int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        sprintf(&md5str[i*2], "%02x", (unsigned int)digarray[i]);
    }

    return strncmp(md5str, sign->data, MD5_DIGEST_LENGTH*2) == 0;
}

static void updateClientName(ksnLNullClass *kl, ksnLNullData *kld, int fd, string_view *name) {
    int wg001_len = strlen(WG001);
    int client_name_len = wg001_len + name->len + 1;//wg001-name + terminating null
    char *client_name = malloc(client_name_len);
    strncpy(client_name, WG001, wg001_len);
    strncpy(client_name + wg001_len, name->data, name->len);
    client_name[client_name_len - 1] = '\0';

    // Remove client with the same name
    int fd_ex = ksnLNullClientIsConnected(kl, client_name);
    if(fd_ex) {
        #ifdef DEBUG_KSNET
        ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);
        ksn_printf(ke, MODULE, DEBUG,"User with name(id): %s is already connected, fd_ex: %d, fd: %d\n", client_name, fd_ex, fd);
        #endif
        if(fd_ex != fd) ksnLNullClientDisconnect(kl, fd_ex, 1);
    }

    // Add client to name map
    if (kld->name) free(kld->name);
    kld->name = client_name;
    kld->name_length = client_name_len;
    if(fd_ex != fd) pblMapAdd(kl->map_n, kld->name, kld->name_length, &fd, sizeof(fd));
}

static void sendConnectedEvent(ksnLNullClass *kl, ksnLNullData *kld) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);
    // TODO: I must use kl->stat.clients or ke->kl->stat.visits ????? instead pblMapSize(kl->map)
    int playload_size = snprintf(0, 0, "{\"client_name\":\"%s\",\"trudp_ip\":\"%s\",\"count_of_clients\":%d}",
        kld->name, kld->t_addr ? kld->t_addr : "error", pblMapSize(kl->map));
    char *payload = malloc(playload_size + 1);
    snprintf(payload, playload_size + 1, "{\"client_name\":\"%s\",\"trudp_ip\":\"%s\",\"count_of_clients\":%d}",
        kld->name, kld->t_addr ? kld->t_addr : "error", pblMapSize(kl->map));

    _send_subscribe_event_connected(ke, payload, playload_size + 1);
    free(payload);
}

static void confirmAuth(ksnLNullClass *kl, ksnLNullData *kld, int fd) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    #ifdef DEBUG_KSNET
        ksn_printf(kev, MODULE, DEBUG,"Confirm auth for user %s\n", kld->name);
    #endif
    // Create L0 packet
    size_t out_data_len = sizeof(teoLNullCPacket) + strlen(ke->teo_cfg.host_name) + 1 + kld->name_length + 1;
    char *out_data = malloc(out_data_len);
    memset(out_data, 0, out_data_len);

    size_t packet_length =
        teoLNullPacketCreate(out_data, out_data_len,
                                CMD_CONFIRM_AUTH, ke->teo_cfg.host_name, (uint8_t*)kld->name, kld->name_length + 1);

    // Send confirmation of the client name
    ssize_t send_size = ksnLNullPacketSend(kl, fd, out_data, packet_length);
    (void)send_size;

    free(out_data);
}

/**
 * Set client name and check it in auth server
 *
 * @param kl Pointer to ksnLNullClass
 * @param kld Pointer to ksnLNullData
 * @param fd TCP client connection file descriptor
 * @param packet Pointer to teoLNullCPacket
 */
static bool ksnLNullClientAuthCheck(ksnLNullClass *kl, ksnLNullData *kld,
        int fd, teoLNullCPacket *packet) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    if (((ksnetEvMgrClass*)kl->ke)->teo_cfg.auth_secret[0] == '\0') {
        ksn_printf(ke, MODULE, DEBUG, "secret not provided, disconnect %d\n", fd);
        ksnLNullClientDisconnect(kl, fd, 1);
        return false;
    }

    uint8_t *payload = teoLNullPacketGetPayload(packet);

    ksn_printf(ke, MODULE, DEBUG,"Checking payload from fd %d:\n%s\n", fd, (const char*)payload);

    //NOTE: string_views will be reseted inside parseAuthPayload
    string_view name;
    string_view sign;
    string_view auth_data_valid_until;

    if (!parseAuthPayload(kl, payload, &name, &sign, &auth_data_valid_until)) {
        ksn_printf(ke, MODULE, DEBUG,"Invalid payload received from fd %d:\n%s\n", fd, (const char*)payload);
        ksnLNullClientDisconnect(kl, fd, 1);
        return false;
    }

    if (!checkAuthData(kl, &name, &auth_data_valid_until, &sign, ((ksnetEvMgrClass*)kl->ke)->teo_cfg.auth_secret)) {
        ksn_printf(ke, MODULE, DEBUG,"Failed to verify signature '%.*s' - '%.*s' - '%.*s' received from fd %d\n",
            name.len,
            name.data,
            auth_data_valid_until.len,
            auth_data_valid_until.data,
            sign.len,
            sign.data,
            fd);
        ksnLNullClientDisconnect(kl, fd, 1);
        return false;
    }

    updateClientName(kl, kld, fd, &name);
    //NOTE: if there're any subs, then we have some work to be done before client enters
    // so we wait for external auth confirm, otherwise confirm instantly
    int connected_subs_num = teoSScrNumberOfEventSubscribers(ke->kc->kco->ksscr, EV_K_L0_CONNECTED);
    if (connected_subs_num > 0) {
        sendConnectedEvent(kl, kld);
    } else {
        confirmAuth(kl, kld, fd);
    }

    return true;
}

/**
 * Send command from this L0 server to all L0 clients
 *
 * @param kl Pointer to ksnLNullClass
 * @param cmd Command
 * @param data_e Pointer to data
 * @param data_e_length Data length
 * @return Number of cliens
 */
static int ksnLNullSendBroadcast(ksnLNullClass *kl, uint8_t cmd, void* data,
        size_t data_length) {

    int num_clients = 0;

    // Send to all clients
    PblIterator *it = pblMapIteratorReverseNew(kl->map);
    if(it != NULL) {
        while(pblIteratorHasPrevious(it) > 0) {
            void *entry = pblIteratorPrevious(it);
            ksnLNullData *client = pblMapEntryValue(entry);
            if(client != NULL && client->name != NULL) {
                int fd = ksnLNullClientIsConnected(kl, client->name);
                if(!fd) continue;
                ksnLNullSend(kl, fd, cmd, data, data_length);
                num_clients++;
            }
        }
        pblIteratorFree(it);
    }
    return num_clients; 
}

/**
 * Send command from this L0 server to L0 client
 *
 * @param kl Pointer to ksnLNullClass
 * @param fd L0 client socket
 * @param cmd Command
 * @param data_e Pointer to data
 * @param data_e_length Data length
 * @return
 */
static ssize_t ksnLNullSend(ksnLNullClass *kl, int fd, uint8_t cmd, void* data,
        size_t data_length) {

    const char *from = ksnetEvMgrGetHostName(kl->ke);
    size_t from_len = strlen(from) + 1;
    // Create L0 packet
    size_t packet_len = teoLNullBufferSize(from_len, data_length);
    char *packet = malloc(packet_len);
    memset(packet, 0, packet_len);

    size_t packet_length =
        teoLNullPacketCreate(packet, packet_len, cmd, from, data, data_length);

    // Send packet
    ssize_t snd = ksnLNullPacketSend(kl, fd, packet, packet_length);
    free(packet);
    return snd;
}

/**
 * Send packet to L0 client
 *
 * @param fd L0 client socket
 * @param pkg Package to send
 * @param pkg_length Package length
 *
 * @return Length of send data or -1 at error
 */
ssize_t ksnLNullPacketSend(ksnLNullClass *kl, int fd, void *pkg,
                           size_t pkg_length) {
    teoLNullCPacket *packet = (teoLNullCPacket *)pkg;
    #ifdef DEBUG_KSNET
    char hexdump[32];
    if (packet->data_length) {
        dump_bytes(hexdump, sizeof(hexdump),
                    teoLNullPacketGetPayload(packet),
                    packet->data_length);
    } else {
        strcpy(hexdump, "(null)");
    }
    #endif

    bool with_encryption = CMD_TRUDP_CHECK(packet->cmd);

    ksnLNullData *kld = ksnLNullGetClientConnection(kl, fd);
    teoLNullEncryptionContext *ctx =
        (with_encryption && (kld != NULL)) ? kld->server_crypt : NULL;

    teoLNullPacketSeal(ctx, with_encryption, packet);

    ssize_t snd = -1;

    // Send by TCP
    if(fd < MAX_FD_NUMBER) {
        teosockSend(fd, pkg, pkg_length);

    } else {    // Send by TR-UDP
        if(kld != NULL) {
            struct sockaddr_storage remaddr;                   ///< Remote address
            socklen_t addrlen = sizeof(remaddr);          ///< Remote address length
            trudpUdpMakeAddr(kld->t_addr, kld->t_port, (__SOCKADDR_ARG) &remaddr, &addrlen);

            #ifdef DEBUG_KSNET
            ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);
            ksn_printf(ke, MODULE, extendedLog(kl),
                       "send packet to trudp addr: %s:%d, cmd = %u, from "
                       "peer: %s, data: %s \n",
                       kld->t_addr, kld->t_port, (unsigned)packet->cmd,
                       packet->peer_name, hexdump);
            #endif

            // Split big packets to smaller
            for(;;) {
                size_t len = pkg_length > 512 ? 512 : pkg_length;
                ksnTRUDPsendto(((ksnetEvMgrClass*)(kl->ke))->kc->ku , 0, 0, 0,
                    packet->cmd, ((ksnetEvMgrClass*)(kl->ke))->kc->ku->fd, pkg, len, 0,
                    (__CONST_SOCKADDR_ARG) &remaddr, addrlen);
                pkg_length -= len;
                if(!pkg_length) break;
                pkg = (uint8_t *)pkg + len;
            }
        }
    }

    // Send packet to client statistic (skip cmd = 0)
    if(packet->cmd) {
        kl->stat.packets_to_client++;
    }

    return snd;
}

/**
 * Connect l0 Server client
 *
 * Called when tcp client connected to TCP l0 Server. Register this client in
 * map
 *
 * @param kl Pointer to ksnLNullClass
 * @param fd TCP client connection file descriptor
 * @param remote_addr remote address string
 * @param remote_port remote port
 */
static void ksnLNullClientConnect(ksnLNullClass *kl, int fd, const char *remote_addr, int remote_port) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    // Set TCP_NODELAY option
    teosockSetTcpNodelay(fd);

    ksn_printf(ke, MODULE, DEBUG_VV,
               "L0 client with fd %d connected from %s:%d\n",
               fd, remote_addr, remote_port);

    // Register client in clients map
    ksnLNullData* kld = ksnLNullClientRegister(kl, fd, remote_addr, remote_port);
    if(kld != NULL) {
        // Create and start TCP watcher (start TCP client processing)
        ev_init (&kld->w, cmd_l0_read_cb);
        ev_io_set (&kld->w, fd, EV_READ);
        kld->w.data = kl;
        ev_io_start (ke->ev_loop, &kld->w);

    } else {
        // Error: can't register TCP fd in tcp proxy map
        // \todo process error: can't register TCP fd in tcp proxy map
        // close(fd);
    }
}

#pragma GCC diagnostic pop

/**
 * Disconnect l0 Server client
 *
 * Called when client disconnected or when the L0 Server closing. Close TCP
 * connections and remove data from L0 Server clients map.
 *
 * @param kl Pointer to ksnLNullClass
 * @param fd TCP client connection file descriptor
 * @param remove_f If true than remove disconnected record from map
 *
 */
void ksnLNullClientDisconnect(ksnLNullClass *kl, int fd, int remove_f) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    size_t valueLength;

    // Get data from L0 Clients map, close TCP watcher and remove this
    // data record from map
    ksnLNullData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &valueLength);
    if(kld != NULL) {

        // Stop L0 client watcher
        if(fd < MAX_FD_NUMBER) {
            ev_io_stop(ke->ev_loop, &kld->w);
            if(remove_f != 2) close(fd);
        }
        if (kld->name != NULL  && !strstr(kld->name, "-new-")) {
            // Show disconnect message
            ksn_printf(ke, MODULE, CONNECT, "### 0005,%s\n", kld->name);
        }

        // L0 statistic - client disconnect
        kl->stat.clients--;

        // Send Disconnect event to all subscribers
        if(kld->name != NULL  && remove_f != 2) {
            int playload_size = snprintf(0, 0, "{\"client_name\":\"%s\",\"trudp_ip\":\"%s\",\"count_of_clients\":%d}",
                kld->name, kld->t_addr ? kld->t_addr : "error", kl->stat.clients);
            char *payload = malloc(playload_size + 1);
            snprintf(payload, playload_size + 1, "{\"client_name\":\"%s\",\"trudp_ip\":\"%s\",\"count_of_clients\":%d}",
                kld->name, kld->t_addr ? kld->t_addr : "error", kl->stat.clients);
            _send_subscribe_event_disconnected(ke, payload, playload_size + 1);
            free(payload);
        }

        // Free name
        if(kld->name != NULL) {

            if(remove_f)
                pblMapRemoveFree(kl->map_n, kld->name, kld->name_length,
                        &valueLength);

            free(kld->name);
            kld->name = NULL;
            kld->name_length = 0;
        }

        // Free TR-UDP addr
        if(kld->t_addr != NULL) {
            free(kld->t_addr);
            kld->t_addr = NULL;
        }

        // Free buffer
        if(kld->read_buffer != NULL) {

            free(kld->read_buffer);
            kld->read_buffer_ptr = 0;
            kld->read_buffer_size = 0;
        }

        if(kld->server_crypt != NULL) {
            free(kld->server_crypt);
            kld->server_crypt = NULL;
        }

        // Remove data from map
        if(remove_f) {
            pblMapRemoveFree(kl->map, &fd, sizeof(fd), &valueLength);
        }
    }
}

/**
 * L0 Server accept callback
 *
 * Register client, create event watchers with clients callback
 *
 * @param loop Event manager loop
 * @param w Pointer to watcher
 * @param revents Events
 * @param fd File description of created client connection
 *
 */
static void cmd_l0_accept_cb(struct ev_loop *loop, struct ev_ksnet_io *w,
                       int revents, int fd) {

    int remote_port = 0;
    const char *remote_addr = NULL;

    struct sockaddr_in adr_inet;
    socklen_t len_inet = sizeof adr_inet;
    bool fail = getpeername(fd, (struct sockaddr *)&adr_inet, &len_inet);
    if (!fail && (len_inet == sizeof adr_inet)) {
        remote_port = (int) ntohs(adr_inet.sin_port);;
        remote_addr = strdup(inet_ntoa(adr_inet.sin_addr));
    }

    ksnLNullClientConnect(w->data, fd, remote_addr, remote_port);
}

#define CHECK_TIMEOUT 10.00
#define SEND_PING_TIMEOUT 30.00
#define DISCONNECT_TIMEOUT 60.00

void _check_connected(uint32_t id, int type, void *data) {

    ksnLNullClass *kl = data;
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    // Disconnect all clients which don't send nothing during timeout
    PblIterator *it = pblMapIteratorReverseNew(kl->map);
    if(it != NULL) {
        while(pblIteratorHasPrevious(it) > 0) {
            void *entry = pblIteratorPrevious(it);
            ksnLNullData *l0_data = pblMapEntryValue(entry);
            int *fd = (int *) pblMapEntryKey(entry);
            // Disconnect client
            if(ksnetEvMgrGetTime(kl->ke) - l0_data->last_time >= DISCONNECT_TIMEOUT) {
                ksn_printf(ke, MODULE, DEBUG, "Disconnect client by timeout, fd: %d, name: %s\n", *fd, l0_data->name);
                ksnLNullClientDisconnect(kl, *fd, 1);
            }
            // Send echo to client, if authorized
            else if(l0_data->name != NULL && ksnetEvMgrGetTime(kl->ke) - l0_data->last_time >= SEND_PING_TIMEOUT) {
                ksn_printf(ke, MODULE, DEBUG, "Send ping to client by timeout, fd: %d, name: %s\n", *fd, l0_data->name);

                // From this host(peer)
                const char *from = ksnetEvMgrGetHostName(kl->ke);
                size_t data_e_length, from_len = strlen(from) + 1;

                // Create echo buffer
                void *data_e = ksnCommandEchoBuffer(((ksnetEvMgrClass *)(kl->ke))->kc->kco, "ping", 5, &data_e_length);

                // Create L0 packet
                size_t out_data_len = sizeof(teoLNullCPacket) + from_len + data_e_length;
                char *out_data = malloc(out_data_len);
                memset(out_data, 0, out_data_len);

                size_t packet_length = teoLNullPacketCreate(
                    out_data, out_data_len, CMD_ECHO, from,
                    data_e, data_e_length);

                ksnLNullPacketSend(kl, *fd, out_data, packet_length);

                free(data_e);
                free(out_data);
            }
        }
        pblIteratorFree(it);
    }

    ksnCQueAdd(kl->cque, _check_connected, CHECK_TIMEOUT, kl);
}

/**
 * Start l0 Server
 *
 * Create and start l0 Server
 *
 * @param kl Pointer to ksnLNullClass
 *
 * @return If return value > 0 than server was created successfully
 */
static int ksnLNullStart(ksnLNullClass *kl) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    int fd = 0;

    if(ke->teo_cfg.l0_allow_f) {

        // Create l0 server at port, which will wait client connections
        int port_created;
        if((fd = ksnTcpServerCreate(
                    ke->kt,
                    ke->teo_cfg.l0_tcp_port,
                    cmd_l0_accept_cb,
                    kl,
                    &port_created)) > 0) {

            ksn_printf(ke, MODULE, MESSAGE,
                    "l0 server fd %d started at port %d\n",
                    fd, port_created);

            ke->teo_cfg.l0_tcp_port = port_created;
            kl->fd = fd;

            // Init check clients cque
            kl->cque = ksnCQueInit(kl->ke, 0);
            ksnCQueAdd(kl->cque, _check_connected, CHECK_TIMEOUT, kl);
        }
    }

    return fd;
}

/**
 * Stop L0 Server
 *
 * Disconnect all connected clients and stop l0 server
 *
 * @param kl Pointer to ksnLNullClass
 *
 */
static void ksnLNullStop(ksnLNullClass *kl) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    // If server started
    if(ke->teo_cfg.l0_allow_f && kl->fd) {

        // Disconnect all clients
        PblIterator *it = pblMapIteratorReverseNew(kl->map);
        if(it != NULL) {
            while(pblIteratorHasPrevious(it) > 0) {
                void *entry = pblIteratorPrevious(it);
                int *fd = (int *) pblMapEntryKey(entry);
                ksnLNullClientDisconnect(kl, *fd, 0);
            }
            pblIteratorFree(it);
        }

        // Clear maps
        pblMapClear(kl->map_n);
        pblMapClear(kl->map);

        ksnCQueDestroy(kl->cque); // Init check clients cque

        // Stop the server
        ksnTcpServerStop(ke->kt, kl->fd);
    }
}

/**
 * Process CMD_L0_CLIENT_BROADCAST teonet command (from peer to l0)
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param rd Pointer to ksnCorePacketData data
 * @return If true - than command was processed by this function
 */
int cmd_l0_broadcast_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd) {
    int retval = 0;
    if(ke->teo_cfg.l0_allow_f) {
        ksnLNullSendBroadcast(ke->kl, rd->cmd, rd->data, rd->data_len);
        retval = 1;
    }
    return retval;
}

/**
 * Process CMD_L0 teonet command
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param rd Pointer to ksnCorePacketData data
 * @return If true - than command was processed by this function
 */
int cmd_l0_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd) {

    int retval = 1;
    ksnLNullSPacket *data = rd->data;

    // Process command
    if(data->cmd == CMD_ECHO || data->cmd == CMD_ECHO_ANSWER ||
       data->cmd == CMD_ECHO_UNRELIABLE || data->cmd == CMD_ECHO_UNRELIABLE_ANSWER ||
       data->cmd == CMD_PEERS || data->cmd == CMD_L0_CLIENTS ||
       data->cmd == CMD_RESET || data->cmd == CMD_SUBSCRIBE || data->cmd == CMD_SUBSCRIBE_RND || data->cmd == CMD_UNSUBSCRIBE ||
       data->cmd == CMD_L0_CLIENTS_N || data->cmd == CMD_L0_STAT ||
       data->cmd == CMD_HOST_INFO || data->cmd == CMD_GET_NUM_PEERS ||
       data->cmd == CMD_TRUDP_INFO ||
       (data->cmd >= CMD_USER && data->cmd < CMD_192_RESERVED) ||
       (data->cmd >= CMD_USER_NR && data->cmd < CMD_LAST)) {

        // \TODO: char *client_name = data->payload;
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
            "got valid command No %d from %s client with %d bytes data ...\n",
            data->cmd, data->payload, data->data_length);
        #endif

        ksnCorePacketData *rds = rd;

        // Process command
        rds->cmd = data->cmd;
        rds->from = data->payload;
        rds->from_len = data->client_name_length;
        rds->data = data->payload + data->client_name_length;
        rds->data_len = data->data_length;
        rds->l0_f = 1;

        // Execute L0 client command
        retval = ksnCommandCheck(ke->kc->kco, rds);
    } else {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
            "%s" "got wrong command No %d from %s client with %d bytes data, "
            "the command skipped ...%s\n",
            ANSI_RED,
            data->cmd, data->payload, data->data_length,
            ANSI_NONE);
        #endif
    }

    return retval;
}

/**
 * Check if L0 client is connected and return it FD
 *
 * @param kl Pointer to ksnLNullClass
 * @param client_name Client name
 *
 * @return Connection fd or NULL ifnot connected
 */
int ksnLNullClientIsConnected(ksnLNullClass *kl, char *client_name) {

    size_t valueLength;
    int *fd = pblMapGet(kl->map_n, client_name, strlen(client_name) + 1,
                &valueLength);

    return fd != NULL ? *fd : 0;
}

/**
 * Process CMD_L0_TO teonet command
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param rd Pointer to ksnCorePacketData data
 * @return If true - than command was processed by this function
 */
int cmd_l0_to_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd) {

    int retval = 1;
    ksnLNullSPacket *data = rd->data;

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, extendedLog(ke->kl),
        "got command No %d for \"%s\" L0 client from peer \"%s\" "
        "with %d bytes data\n",
        data->cmd, data->payload, rd->from, data->data_length);
    #endif

    // Got packet from peer statistic
    ke->kl->stat.packets_from_peer++;

    // If l0 module is initialized
    if(ke->kl) {

        int fd = ksnLNullClientIsConnected(ke->kl, data->payload);
        if (fd) {

            // Create L0 packet
            size_t out_data_len = sizeof(teoLNullCPacket) + rd->from_len +
                    data->data_length;
            char *out_data = malloc(out_data_len);
            memset(out_data, 0, out_data_len);
            size_t packet_length = teoLNullPacketCreate(out_data, out_data_len,
                    data->cmd, rd->from, (const uint8_t*)data->payload + data->client_name_length,
                    data->data_length);

            // Send command to L0 client
            ssize_t snd = ksnLNullPacketSend(ke->kl, fd, out_data, packet_length);
            (void)snd;

            #ifdef DEBUG_KSNET
            teoLNullCPacket *packet = (teoLNullCPacket *)out_data;
            //void *packet_data = packet->peer_name + packet->peer_name_length;
            ksn_printf(ke, MODULE, DEBUG_VV,
                "send %d bytes to \"%s\" L0 client: %d bytes data, "
                "from peer \"%s\"\n",
                (int)snd, data->payload,
                packet->data_length, packet->peer_name);
            #endif
            free(out_data);
        }

        // The L0 client was disconnected
        else {

            #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, DEBUG_VV,
                "%s" "the \"%s\" L0 client has not connected to the server%s\n",
                ANSI_RED, data->payload, ANSI_NONE);
            #endif
        }
    }

    // If l0 server is no initialized
    else {
        #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, ERROR_M,
                "can't resend command %d to L0 client \"%s\" from peer \"%s\": " "%s" "the L0 module has not been initialized at this host%s\n",
                data->cmd, data->payload, rd->from, ANSI_RED, ANSI_NONE);
        #endif
    }

    return retval;
}

/**
 * JSON request parameters structure
 *
 * @member userId
 * @member clientId
 * @member username
 * @member accessToken
 */
typedef struct json_param {

    char *userId;
    char *clientId;
    char *username;
    char *accessToken;
    char *networks;

} json_param;

/**
 * Check json key equalize to string
 *
 * @param json Json string
 * @param tok Jsmt json token
 * @param s String to compare
 *
 * @return Return 0 if found
 */
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {

    if(/*(tok->type == JSMN_STRING || tok->type == JSMN_ARRAY) && */
        (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {

        return 0;
    }

    return -1;
}

/**
 * Parse JSON request
 *
 * @param json_str
 * @param jp
 *
 * @return Parsed if true, or 0 if error
 */
static int json_parse(char *data, json_param *jp) {

    size_t data_length = strlen(data);
    size_t num_of_tags = get_num_of_tags(data, data_length);

    // Get gags buffer
    jsmntok_t *t = malloc(num_of_tags * sizeof(jsmntok_t));

    // Parse json
    jsmn_parser p;
    jsmn_init(&p);
    int r = jsmn_parse(&p, data, data_length, t, num_of_tags);
    if(r < 0) {

        // This is not JSON string - skip processing
        // printf("Failed to parse JSON: %d\n", r);
        free(t);
        return 0;
    }

    // Assume the top-level element is an object
    if (r < 1 || t[0].type != JSMN_OBJECT) {

        // This is not JSON object - skip processing
        // printf("Object expected\n");
        free(t);
        return 0;
    }

    // Teonet db json string format:
    //
    // {"key":"Test-22","data":"Test data"}
    // or
    // {"key":"Test-22"}
    //
    enum KEYS {
        USERID = 0x1,
        CLIENTID = 0x2, // 0x2
        USERNAME = 0x4, // 0x4
        ACCESSTOKEN = 0x8, // 0x8
        NETWORKS = 0x10, // 0x10

        ALL_KEYS = USERID | CLIENTID | USERNAME | ACCESSTOKEN | NETWORKS
    };
    const char *USERID_TAG = "userId";
    const char *CLIENTID_TAG = "clientId";
    const char *USERNAME_TAG = "username";
    const char *ACCESSTOKEN_TAG = "accessToken";
    const char *NETWORKS_TAG = "networks";
    memset(jp, 0, sizeof(*jp)); // Set JSON parameters to NULL
    int i, keys = 0;
    // Loop over json keys of the root object and find needle: cmd, to, cmd_data
    for (i = 1; i < r && keys != ALL_KEYS; i++) {

        // Find USERID tag
        #define find_tag(VAR, KEY, TAG) \
        if(!(keys & KEY) && jsoneq(data, &t[i], TAG) == 0) { \
            \
            VAR = strndup((char*)data + t[i+1].start, \
                    t[i+1].end-t[i+1].start); \
            keys |= KEY; \
            i++; \
        }

        // How tag key and skipped tag data
        //printf("tag: %.*s\n", t[i].end - t[i].start,  (char*)data + t[i].start);

        // Find USERID tag
        find_tag(jp->userId, USERID, USERID_TAG)

        // Find CLIENTID tag
        else find_tag(jp->clientId, CLIENTID, CLIENTID_TAG)

        // Find USERNAME tag
        else find_tag(jp->username, USERNAME, USERNAME_TAG)

        // Find ACCESSTOKEN tag
        else find_tag(jp->accessToken, ACCESSTOKEN, ACCESSTOKEN_TAG)

        // Find NETWORKS tag
        else find_tag(jp->networks, NETWORKS, NETWORKS_TAG)
    }
    free(t);

    return 1;
}

/**
 * Check l0 client answer from authentication application
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return
 */
int cmd_l0_check_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    // Parse request
    char *json_data_unesc = rd->data;
    json_param jp;
    json_parse(json_data_unesc, &jp);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG,
        "got %d bytes auth confirmation from %s authentication application, "
            "username: %s\n%s\n",
        rd->data_len, rd->from, jp.userId, rd->data
    );
    #endif

    ksnLNullClass *kl = ke->kl;

    // Authorize new user
    int fd = -1;
    
    if(jp.userId && (fd = ksnLNullClientIsConnected(kl, jp.userId))) {
        ksnLNullData* kld = pblMapGet(kl->map, &fd, sizeof(fd), NULL);
        if(kld != NULL) {
            confirmAuth(kl, kld, fd);
        } else {
            #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, DEBUG,
                "can't confirm auth of the client %s. kld not found.\n",
                jp.userId
            );
            #endif
        }
    } else {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG,
            "can't confirm auth of the client %s. Client not connected.\n",
            jp.userId
        );
        #endif
    }

    // Free json tags
    if(jp.accessToken != NULL) free(jp.accessToken);
    if(jp.clientId != NULL) free(jp.clientId);
    if(jp.userId != NULL) free(jp.userId);
    if(jp.username != NULL) free(jp.username);
    if(jp.networks != NULL) free(jp.networks);

    return 1;
}

/**
 * Kick l0 client
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return
 */
int cmd_l0_kick_client(ksnCommandClass *kco, ksnCorePacketData *rd) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    // Parse request
    char *json_data_unesc = rd->data;
    json_param jp;
    json_parse(json_data_unesc, &jp);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG,
        "got %d bytes kick command from %s authentication application, "
            "username: %s\n%s\n",
        rd->data_len, rd->from, jp.userId, rd->data
    );
    #endif

    ksnLNullClass *kl = ke->kl;

    // Remove old user
    int fd = -1;
    if(jp.userId && (fd = ksnLNullClientIsConnected(kl, jp.userId))) {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG, "User with name(id): %s is already connected, fd: %d\n", jp.userId, fd);
        #endif

        // Send 99 command to user
        ksnLNullSend(kl, fd, CMD_L0_CLIENT_RESET, "2", 2);

        // Check user already connected
        size_t valueLength;
        ksnLNullData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &valueLength);
        if(kld != NULL) {
            ksnLNullClientDisconnect(kl, fd, 2);
        }
    }

    // Free json tags
    if(jp.accessToken != NULL) free(jp.accessToken);
    if(jp.clientId != NULL) free(jp.clientId);
    if(jp.userId != NULL) free(jp.userId);
    if(jp.username != NULL) free(jp.username);
    if(jp.networks != NULL) free(jp.networks);

    return 1;
}

/**
 * Create list of clients
 *
 * @param kl
 * @return
 */
teonet_client_data_ar *ksnLNullClientsList(ksnLNullClass *kl) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    teonet_client_data_ar *data_ar = NULL;

    if(kl != NULL && ke->teo_cfg.l0_allow_f && kl->fd) {

        uint32_t length = pblMapSize(kl->map);
        data_ar = malloc(sizeof(teonet_client_data_ar) +
                length * sizeof(data_ar->client_data[0]));
        int i = 0;

        // Create clients list
        PblIterator *it = pblMapIteratorReverseNew(kl->map);
        if(it != NULL) {
            while(pblIteratorHasPrevious(it) > 0) {
                void *entry = pblIteratorPrevious(it);
                //int *fd = (int *) pblMapEntryKey(entry);
                ksnLNullData *data = pblMapEntryValue(entry);
                if(data != NULL && data->name != NULL) {
                    strncpy(data_ar->client_data[i].name, data->name,
                            sizeof(data_ar->client_data[i].name));
                    i++;
                }
            }
            pblIteratorFree(it);
        }
        data_ar->length = i;
    }

    return data_ar;
}

/**
 * Return size of teonet_client_data_ar data
 *
 * @param clients_data
 * @return Size of teonet_client_data_ar data
 */
inline size_t ksnLNullClientsListLength(teonet_client_data_ar *clients_data) {

    return clients_data != NULL ?
        sizeof(teonet_client_data_ar) +
            clients_data->length * sizeof(clients_data->client_data[0]) : 0;
}

#define BUFFER_SIZE_CLIENT 4096

static ssize_t packetCombineClient(trudpChannelData *tcd, char *data, size_t data_len, ssize_t recieved)
{
    ssize_t retval = -1;

    if (tcd->last_packet_ptr > 0) {
        tcd->read_buffer_ptr -= tcd->last_packet_ptr;
        if (tcd->read_buffer_ptr > 0) {
            memmove(tcd->read_buffer, (char *)tcd->read_buffer + tcd->last_packet_ptr, (int)tcd->read_buffer_ptr);
        }

        tcd->last_packet_ptr = 0;
    }

    if ((size_t) recieved > tcd->read_buffer_size - tcd->read_buffer_ptr) {
        tcd->read_buffer_size += data_len;
        if (tcd->read_buffer) {
            tcd->read_buffer = realloc(tcd->read_buffer, tcd->read_buffer_size);
        } else {
            tcd->read_buffer = malloc(tcd->read_buffer_size);
        }
    }

    if (recieved > 0) {
        static const size_t headerSize = sizeof(teoLNullCPacket) - 
            sizeof(((teoLNullCPacket *)data)->header_checksum);

        if (tcd->read_buffer_ptr == 0 &&
            ((teoLNullCPacket *)data)->header_checksum != get_byte_checksum( (uint8_t*) data, headerSize)) {
            if (!tcd->stat.packets_send && !tcd->stat.packets_receive) {
                // trudpChannelDestroy(tcd);
            }
            return -3;
        }
        memmove((char*)tcd->read_buffer + tcd->read_buffer_ptr, data, recieved);
        tcd->read_buffer_ptr += recieved;
    }

    teoLNullCPacket *packet = (teoLNullCPacket *)tcd->read_buffer;
    ssize_t len;

    // Process read buffer
    if(tcd->read_buffer_ptr - tcd->last_packet_ptr > sizeof(teoLNullCPacket) &&
       tcd->read_buffer_ptr - tcd->last_packet_ptr >= (size_t)(len = sizeof(teoLNullCPacket) + packet->peer_name_length + packet->data_length)) {

        // Check checksum
        uint8_t header_checksum = get_byte_checksum((const uint8_t*)packet, sizeof(teoLNullCPacket) - sizeof(packet->header_checksum));
        uint8_t checksum = get_byte_checksum((const uint8_t*)packet->peer_name, packet->peer_name_length + packet->data_length);

        if(packet->header_checksum == header_checksum && packet->checksum == checksum) {
            // Packet has received - return packet size
            retval = len;
            tcd->last_packet_ptr += len;
        } else { // Wrong checksum, wrong packet - drop this packet and return -2
            tcd->read_buffer_ptr = 0;
            tcd->last_packet_ptr = 0;
            retval = -2;
        }
    }

    return retval;
}

ssize_t recvCheck(trudpChannelData *tcd, char *data, ssize_t data_length)
{
    return packetCombineClient(tcd, data, BUFFER_SIZE_CLIENT, data_length != -1 ? data_length : 0);
}

/**
 * Create and send key exchange packet to client
 * in @a kld must be initialized and valid encription context
 * 
 * @param kl L0 module
 * @param kld client connection
 * @param fd connection id
 * 
 * @return true if succeed
*/
static bool sendKEXResponse(ksnLNullClass *kl, ksnLNullData *kld, int fd) {
    size_t resp_len = teoLNullKEXBufferSize(kld->server_crypt->enc_proto);
    uint8_t *resp_buf = (uint8_t *)malloc(resp_len);

    bool ok = teoLNullKEXCreate(kld->server_crypt, resp_buf, resp_len) != 0;
    if (ok) {
        const int CMD_L0_KEX_ANSWER = 0;
        ksnLNullSend(kl, fd, CMD_L0_KEX_ANSWER, resp_buf, resp_len);
    }
    free(resp_buf);
    return ok;
}

/**
 * Handle receives key exchange packet
 * Creates encryption context if not created before
 * Checks if encryption context is compatible with received packet
 * Applies client keys and ensures encrypted session established
 *
 * @param kl L0 module
 * @param kld client connection
 * @param fd connection id
 * @param kex key exchange payload
 * @param kex_length payload length
 *
 * @return true if succeed, in case of failure calling code supposed to
 * disconnect user
 */
static bool processKeyExchange(ksnLNullClass *kl, ksnLNullData *kld, int fd,
                               KeyExchangePayload_Common *kex,
                               size_t kex_length) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);
    (void)ke;
    if (kld->server_crypt == NULL) {
        // Create compatible encryption context
        size_t crypt_size = teoLNullEncryptionContextSize(kex->protocolId);
        if (crypt_size == 0) {
            #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, ERROR_M,
                       "Invalid KEX from %s:%d - can't create ctx(%d); disconnecting\n",
                       kld->t_addr, kld->t_port, (int)kex->protocolId);
            #endif
            return false;
        }
        kld->server_crypt = (teoLNullEncryptionContext *)malloc(crypt_size);
        teoLNullEncryptionContextCreate(kex->protocolId,
                                        (uint8_t *)kld->server_crypt,
                                        crypt_size);
    }

    // check if can be applied
    if (!teoLNullKEXValidate(kld->server_crypt, kex, kex_length)) {
        #ifdef DEBUG_KSNET
        ksn_printf(
            ke, MODULE, ERROR_M,
            "Invalid KEX from %s:%d - failed validation; disconnecting\n",
            kld->t_addr, kld->t_port);
        #endif
        return false;
    }

    // check if applied successfully
    if (!teoLNullEncryptionContextApplyKEX(kld->server_crypt, kex,
                                           kex_length)) {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, ERROR_M,
                   "Invalid KEX from %s:%d - can't apply; disconnecting\n",
                   kld->t_addr, kld->t_port);
        #endif
        return false;
    }

    // Key exchange payload should be sent unencrypted
    kld->server_crypt->state = SESCRYPT_PENDING;
    if (!sendKEXResponse(kl, kld, fd)) {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, ERROR_M,
                   "Failed to send KEX response to %s:%d; disconnecting\n",
                   kld->t_addr, kld->t_port);
        #endif
        return false;
    }

    // Success !!!
    kld->server_crypt->state = SESCRYPT_ESTABLISHED;
    return true;
}

/*
 * Process small or lagre packet
 * 
 * @param kl L0 module
 * @param kld client connection
 * @param rd pointer to received data
 * @param tcd pointer to trudpChannelData
 * @param packet pointer to teoLNullCPacket
 * @param packet_kind kind of packet string used in logs
 * 
 * @return true if processed
 */
static int processPacket(ksnLNullClass *kl, ksnLNullData *kld,
                      ksnCorePacketData *rd, trudpChannelData *tcd,
                      teoLNullCPacket *packet, const char *packet_kind) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    const bool was_encrypted = teoLNullPacketIsEncrypted(packet);
    if (!teoLNullPacketDecrypt(kld->server_crypt, packet)) {
        ksn_printf(ke, MODULE, ERROR_M,
                   "teoLNullPacketDecrypt failed fd %d/ ctx %p\n", tcd->fd,
                   kld->server_crypt);
        return 0;
    }

    teoLNullPacketCheckMiscrypted(kl, kld, packet);

    #ifdef DEBUG_KSNET
    uint8_t *data = teoLNullPacketGetPayload(packet);
    char hexdump[32];
    if (packet->data_length) {
        dump_bytes(hexdump, sizeof(hexdump), data, packet->data_length);
    } else {
        strcpy(hexdump, "(null)");
    }
    const char *str_enc = was_encrypted ? " encrypted" : "";
    ksn_printf(ke, MODULE, extendedLog(kl),
               "got %s%s trudp packet, from: %s:%d, fd: %d, "
               "cmd: %u, to peer: %s, data: %s\n",
               packet_kind, str_enc, rd->addr, rd->port, tcd->fd,
               (unsigned)packet->cmd, packet->peer_name, hexdump);
    #endif

    // Got packet from client statistic (skip cmd = 0)
    if(packet->cmd) {
        kl->stat.packets_from_client++;
    }

    switch (packet->cmd) {

    // Login packet
    case 0: {
        if (packet->peer_name_length == 1 && !packet->peer_name[0] &&
            packet->data_length) {
            uint8_t *l0_data = teoLNullPacketGetPayload(packet);
            KeyExchangePayload_Common *kex =
                teoLNullKEXGetFromPayload(l0_data, packet->data_length);
            if (kex) {
                // received client keys
                // must be first time or exactly the same as previous
                if (!processKeyExchange(kl, kld, tcd->fd, kex,
                                        packet->data_length)) {
                    // Failed. disconnect user
                    trudp_ChannelSendReset(tcd);
                    ksnLNullClientDisconnect(kl, tcd->fd, 1);

                } else {
                    char saltdump[62];
                    uint8_t *salt = kld->server_crypt->keys.sessionsalt.data;
                    dump_bytes(saltdump, sizeof(saltdump), salt, sizeof(salt));
                    ksn_printf(ke, MODULE, DEBUG_VV,
                               "got valid kex from %s:%d salt %s state %d\n",
                               kld->t_addr, kld->t_port, saltdump,
                               kld->server_crypt->state);
                }
            } else {
                // process login here
                size_t expected_len = packet->data_length - 1;

                if (expected_len > 0 && data[expected_len] == 0 &&
                    strlen((const char *)data) == expected_len) {

                    if (!ksnLNullClientAuthCheck(kl, kld, tcd->fd, packet)) {
                        trudp_ChannelSendReset(tcd);
                    }
                } else {
                    ksn_printf(ke, MODULE, ERROR_M,
                               "got invalid login packet from %s:%d\n", 
                               kld->t_addr, kld->t_port);
                    // Failed. disconnect user
                    trudp_ChannelSendReset(tcd);
                    ksnLNullClientDisconnect(kl, tcd->fd, 1);
                }
            }
            return 1;
        }
        return 0;
    } break;

    // Process other L0 packets
    default: {
        // Process other L0 TR-UDP packets
        // Send packet to peer
        if (kld->name != NULL) {
            kld->last_time = ksnetEvMgrGetTime(kl->ke);
            ksnLNullSendFromL0(kl, packet, kld->name, kld->name_length);
        } else {
            //TODO: do we want to disconnect client here?
            trudp_ChannelSendReset(tcd);
        }
        return 1;
    }
    }

    return 0;
}

/**
 * Check and process L0 packet received through TR-UDP
 *
 * @param data
 * @param data_len
 * @return
 */
int ksnLNulltrudpCheckPaket(ksnLNullClass *kl, ksnCorePacketData *rd) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kl);

    trudpChannelData *tcd = trudpGetChannelAddr(ke->kc->ku, rd->addr, rd->port, 0);
    if (tcd == NULL || tcd == (void *)-1) {
        return 1;
    }

    if(tcd->fd == 0) {
        // Add fd to tr-udp channel data
        tcd->fd = ksnLNullGetNextFakeFd(kl);
    }

    teoLNullCPacket *packet_sm = teoLNullPacketGetFromBuffer(rd->data, rd->data_len);
    if (packet_sm != NULL) {
        ksnLNullData *kld = ksnLNullGetClientConnection(kl, tcd->fd);
        if (kld == NULL) {
            if (rd->cmd != 0) {
                ksnLNullClientDisconnect(kl, tcd->fd, 1);
                return 0;
            }
            kld = ksnLNullClientRegister(kl, tcd->fd, rd->addr, rd->port);
        }
        return processPacket(kl, kld, rd, tcd, packet_sm, "small"); // Small packet
    }

    // FIXME FIXME FIXME in recvCheck ---> packetCombineClient we could use teoLNullPacketGetFromBuffer
    ssize_t rc = recvCheck(tcd, rd->data, rd->data_len);
    if (rc == -3) {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV, "WRONG UDP PACKET %d\n", rc);
        #endif
        return 1; // TODO: Check this return value (may be it should be 0)
    } else if (rc <= 0) {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
                    "Wait next part of large packet %d status = %d\n",
                    rd->data_len, rc
        );
        #endif
        return 1;
    }

    // rc > 0 ensures it's correct teoLNullCPacket
    teoLNullCPacket *packet_large = (teoLNullCPacket *)tcd->read_buffer;
    ksnLNullData *kld = ksnLNullGetClientConnection(kl, tcd->fd);
    if (kld == NULL) {
        kld = ksnLNullClientRegister(kl, tcd->fd, rd->addr, rd->port);
    }
    return processPacket(kl, kld, rd, tcd, packet_large, "large"); // Large packet
}

/**
 * Returns L0 client encryption context and return it if available
 *
 * @param kl Pointer to ksnLNullClass
 * @param fd Client name connection fd
 *
 * @return teoLNullEncryptionContext pointer or null if not available
 */
teoLNullEncryptionContext *ksnLNullClientGetCrypto(ksnLNullClass *kl, int fd) {
    ksnLNullData *kld = ksnLNullGetClientConnection(kl, fd);
    return (kld == NULL) ? NULL : kld->server_crypt;
}

