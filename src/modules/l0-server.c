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
#include <string.h>

#include "jsmn.h"
#include "ev_mgr.h"
#include "l0-server.h"
#include "utils/rlutil.h"

#define MODULE _ANSI_LIGHTCYAN "l0_server" _ANSI_NONE
#define TEO_AUTH "teo-auth"

// Local functions
static int ksnLNullStart(ksnLNullClass *kl);
static void ksnLNullStop(ksnLNullClass *kl);
static void ksnLNullClientDisconnect(ksnLNullClass *kl, int fd, int remove_f);
static ksnet_arp_data *ksnLNullSendFromL0(ksnLNullClass *kl, teoLNullCPacket *packet,
        char *cname, size_t cname_length);

// Other modules not declared functions
void *ksnCoreCreatePacket(ksnCoreClass *kc, uint8_t cmd, const void *data,
        size_t data_len, size_t *packet_len);
#include "tr-udp_.h"  // ksnTRUDPmakeAddr

// External constants
extern const char *localhost;

/**
 * Pointer to ksnetEvMgrClass
 *
 */
#define kev ((ksnetEvMgrClass*)kl->ke)
#define L0_VERSION 0 ///< L0 Server version

/**
 * Initialize ksnLNull module [class](@ref ksnLNullClass)
 *
 * @param ke Pointer to ksnetEvMgrClass
 *
 * @return Pointer to created ksnLNullClass or NULL if error occurred
 */
ksnLNullClass *ksnLNullInit(void *ke) {

    ksnLNullClass *kl = NULL;

    if(((ksnetEvMgrClass*)ke)->ksn_cfg.l0_allow_f) {

        kl = malloc(sizeof(ksnLNullClass));
        if(kl != NULL)  {
            kl->ke = ke; // Pointer event manager class
            kl->map = pblMapNewHashMap(); // Create a new hash map
            kl->map_n = pblMapNewHashMap(); // Create a new hash map
            memset(&kl->stat, 0, sizeof(kl->stat)); // Clear statistic data
            ksnLNullStart(kl); // Start L0 Server
        }
    }

    return kl;
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

    size_t data_len = KSN_BUFFER_DB_SIZE; // Buffer length
    ksnLNullClass *kl = w->data; // Pointer to ksnLNullClass
    char data[data_len]; // Buffer

    // Read TCP data
    ssize_t received = read(w->fd, data, data_len);
    #ifdef DEBUG_KSNET
    ksn_printf(kev, MODULE, DEBUG_VV,
            "Got something from fd %d w->events = %d, received = %d ...\n",
            w->fd, w->events, (int)received);
#endif

    // Disconnect client:
    // Close TCP connections and remove data from l0 clients map
    if(!received) {

        #ifdef DEBUG_KSNET
        ksn_printf(kev, MODULE, DEBUG_VV,
            "Connection closed. Stop listening fd %d ...\n",
            w->fd
        );
        #endif

        ksnLNullClientDisconnect(kl, w->fd, 1);
    }

    // \todo Process reading error
    else if (received < 0) {

        //        if( errno == EINTR ) {
        //            // OK, just skip it
        //        }

//        #ifdef DEBUG_KSNET
//        ksnet_printf(
//            &kev->ksn_cfg, DEBUG,
//            "%sl0 Server:%s "
//            "Read error ...%s\n",
//            ANSI_LIGHTCYAN, ANSI_RED, ANSI_NONE
//        );
//        #endif
    }

    // Success read. Process package and resend it to teonet
    else {

        size_t vl;
        ksnLNullData* kld = pblMapGet(kl->map, &w->fd, sizeof(w->fd), &vl);
        if(kld != NULL) {

            // Add received data to the read buffer
            if(received > kld->read_buffer_size - kld->read_buffer_ptr) {

                // Increase read buffer size
                kld->read_buffer_size += data_len; //received;
                if(kld->read_buffer != NULL)
                    kld->read_buffer = realloc(kld->read_buffer,
                            kld->read_buffer_size);
                else
                    kld->read_buffer = malloc(kld->read_buffer_size);

                #ifdef DEBUG_KSNET
                ksn_printf(kev, MODULE, DEBUG_VV,
                    "%s" "increase read buffer to new size: %d bytes ...%s\n",
                    ANSI_DARKGREY, kld->read_buffer_size,
                    ANSI_NONE);
                #endif
            }
            memmove(kld->read_buffer + kld->read_buffer_ptr, data, received);
            kld->read_buffer_ptr += received;

            teoLNullCPacket *packet = (teoLNullCPacket *)kld->read_buffer;
            size_t len, ptr = 0;

            // \todo Check packet

            // Process read buffer
            while(kld->read_buffer_ptr - ptr >= (len = sizeof(teoLNullCPacket) +
                    packet->peer_name_length + packet->data_length)) {

                    // Check checksum
                    uint8_t header_checksum = get_byte_checksum(packet,
                            sizeof(teoLNullCPacket) -
                            sizeof(packet->header_checksum));
                    uint8_t checksum = get_byte_checksum(packet->peer_name,
                            packet->peer_name_length + packet->data_length);
                    if(packet->header_checksum == header_checksum &&
                       packet->checksum == checksum) {

                        // Check initialize packet:
                        // cmd = 0, to_length = 1, data_length = 1 + data_len,
                        // data = client_name
                        //
                        if(packet->cmd == 0 && packet->peer_name_length == 1 &&
                                !packet->peer_name[0] && packet->data_length) {

                            // Set temporary name (it will be changed after TEO_AUTH answer)
                            kld->name = strdup(packet->peer_name + 1);
                            kld->name_length = strlen(kld->name) + 1;
                            pblMapAdd(kl->map_n, kld->name, kld->name_length,
                                    &w->fd, sizeof(w->fd));

                            // Send login to authentication application
                            // to check this client 
                            ksnCoreSendCmdto(kev->kc, TEO_AUTH, CMD_USER,
                                    kld->name, kld->name_length);

                            // Login will continue when answer received
                            #ifdef DEBUG_KSNET
                            ksn_printf(kev, MODULE, DEBUG,
                                "got login command with name %s\n", kld->name
                            );
                            #endif
                                                        
                        }

                        // Resend data to teonet
                        else {

                            ksnLNullSendFromL0(kl, packet, kld->name,
                                    kld->name_length);
                        }
                    }

                    // Wrong checksum - drop this packet
                    else {
                        #ifdef DEBUG_KSNET
                        ksn_printf(kev, MODULE, DEBUG_VV,
                            "%s" "wrong packet %d bytes length; dropped ...%s\n",
                            ANSI_RED, len, ANSI_NONE);
                        #endif

                        kld->read_buffer_ptr = 0;
                    }

                ptr += len;
                packet = (void*)packet + len;
            }

            // Check end of buffer
            if(kld->read_buffer_ptr - ptr > 0) {

                #ifdef DEBUG_KSNET
                ksn_printf(kev, MODULE, DEBUG_VV,
                    "%s" "wait next part of packet, now it has %d bytes ...%s\n",
                    ANSI_DARKGREY, kld->read_buffer_ptr - ptr,
                    ANSI_NONE);
                #endif

                kld->read_buffer_ptr = kld->read_buffer_ptr - ptr;
                memmove(kld->read_buffer, kld->read_buffer + ptr,
                        kld->read_buffer_ptr);
            }
            else kld->read_buffer_ptr = 0;
        }
    }
}

// Get L0 server statistic
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

    size_t out_data_len = sizeof(ksnLNullSPacket) + cname_length +
            packet->data_length;
    char *out_data = malloc(out_data_len);
    memset(out_data, 0, out_data_len);
    ksnLNullSPacket *spacket = (ksnLNullSPacket*) out_data;

    // Create teonet L0 packet
    spacket->cmd = packet->cmd;
    spacket->from_length = cname_length;
    memcpy(spacket->from, cname, cname_length);
    spacket->data_length = packet->data_length;
    memcpy(spacket->from + spacket->from_length,
        packet->peer_name + packet->peer_name_length, spacket->data_length);

    // Send teonet L0 packet
    ksnet_arp_data *arp = NULL;
    // Send to peer
    if(strcmp((char*)packet->peer_name, ksnetEvMgrGetHostName(kev))) {

        #ifdef DEBUG_KSNET
        ksn_printf(kev, MODULE, DEBUG_VV,
            "send command from L0 \"%s\" client to \"%s\" peer ...\n",
            spacket->from, packet->peer_name);
        #endif

        arp = ksnCoreSendCmdto(kev->kc, packet->peer_name, CMD_L0,
                spacket, out_data_len);
    }
    // Send to this host
    else {
        #ifdef DEBUG_KSNET
        ksn_printf(kev, MODULE, DEBUG_VV,
            "send command to L0 server peer \"%s\" from L0 client \"%s\" ...\n",
            packet->peer_name, spacket->from);
        #endif

        // Create packet
        size_t pkg_len;
        void *pkg = ksnCoreCreatePacket(kev->kc, CMD_L0, spacket, out_data_len,
                    &pkg_len);
        struct sockaddr_in addr;             // address structure
        socklen_t addrlen = sizeof(addr);    // address structure length
        if(!make_addr(localhost, kev->kc->port, (__SOCKADDR_ARG) &addr,
                &addrlen)) {

            ksnCoreProcessPacket(kev->kc, pkg, pkg_len, (__SOCKADDR_ARG) &addr);
            arp = ksnetArpGet(kev->kc->ka, (char*)packet->peer_name);
        }
        free(pkg);
    }

    free(out_data);

    return arp;
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

    // Create L0 packet
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
        spacket->from);
    #endif
    int rv = ksnCoreSendto(((ksnetEvMgrClass*)ke)->kc, addr, port, CMD_L0_TO,
            out_data, out_data_len);

    free(out_data);

    return rv;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

/**
 * Connect l0 Server client
 *
 * Called when tcp client connected to TCP l0 Server. Register this client in
 * map
 *
 * @param kl Pointer to ksnLNullClass
 * @param fd TCP client connection file descriptor
 *
 */
static void ksnLNullClientConnect(ksnLNullClass *kl, int fd) {

    // Set TCP_NODELAY option
    set_tcp_nodelay(fd);

    ksn_printf(kev, MODULE, CONNECT, "L0 client with fd %d connected\n", fd);

    // Send Connected event to all subscribers
    //teoSScrSend(kev->kc->kco->ksscr, EV_K_L0_CONNECTED, "", 1, 0);

    // Register client in clients map
    ksnLNullData data;
    data.name = NULL;
    data.name_length = 0;
    data.read_buffer = NULL;
    data.read_buffer_ptr = 0;
    data.read_buffer_size = 0;
    pblMapAdd(kl->map, &fd, sizeof(fd), &data, sizeof(ksnLNullData));

    // Create and start TCP watcher (start client processing)
    size_t valueLength;
    ksnLNullData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &valueLength);
    if(kld != NULL) {

        // Create and start TCP watcher (start TCP client processing)
        ev_init (&kld->w, cmd_l0_read_cb);
        ev_io_set (&kld->w, fd, EV_READ);
        kld->w.data = kl;
        ev_io_start (kev->ev_loop, &kld->w);
    }

    // Error: can't register TCP fd in tcp proxy map
    else {
        // \todo process error: can't register TCP fd in tcp proxy map
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
static void ksnLNullClientDisconnect(ksnLNullClass *kl, int fd, int remove_f) {

    size_t valueLength;

    // Get data from L0 Clients map, close TCP watcher and remove this
    // data record from map
    ksnLNullData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &valueLength);
    if(kld != NULL) {

        // Stop L0 client watcher
        ev_io_stop(kev->ev_loop, &kld->w);
        close(fd);

        // Show disconnect message
        ksn_printf(kev, MODULE, CONNECT,
            "L0 client with fd %d disconnected\n", fd);

        // Send Disconnect event to all subscribers
        teoSScrSend(kev->kc->kco->ksscr, EV_K_L0_DISCONNECTED, kld->name,
                kld->name_length, 0);

        // Free name
        if(kld->name != NULL) {

            if(remove_f)
                pblMapRemoveFree(kl->map_n, kld->name, kld->name_length,
                        &valueLength);

            free(kld->name);
            kld->name = NULL;
            kld->name_length = 0;
        }

        // Free buffer
        if(kld->read_buffer != NULL) {

            free(kld->read_buffer);
            kld->read_buffer_ptr = 0;
            kld->read_buffer_size = 0;
        }

        // Remove data from map
        if(remove_f)
            pblMapRemoveFree(kl->map, &fd, sizeof(fd), &valueLength);
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

    ksnLNullClientConnect(w->data, fd);
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

    int fd = 0;

    if(kev->ksn_cfg.l0_allow_f) {

        // Create l0 server at port, which will wait client connections
        int port_created;
        if((fd = ksnTcpServerCreate(
                    kev->kt,
                    kev->ksn_cfg.l0_tcp_port,
                    cmd_l0_accept_cb,
                    kl,
                    &port_created)) > 0) {

            ksn_printf(kev, MODULE, MESSAGE,
                    "l0 server fd %d started at port %d\n",
                    fd, port_created);

            kev->ksn_cfg.l0_tcp_port = port_created;
            kl->fd = fd;
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

    // If server started
    if(kev->ksn_cfg.l0_allow_f && kl->fd) {

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

        // Stop the server
        ksnTcpServerStop(kev->kt, kl->fd);
    }
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
    if(data->cmd == CMD_ECHO || data->cmd == CMD_PEERS ||
       data->cmd == CMD_L0_CLIENTS || data->cmd == CMD_RESET ||
       data->cmd == CMD_SUBSCRIBE || data->cmd == CMD_L0_CLIENTS_N ||
       data->cmd == CMD_L0_STAT || data->cmd == CMD_HOST_INFO ||
       data->cmd == CMD_GET_NUM_PEERS || data->cmd == CMD_TRUDP_INFO ||
       (data->cmd >= CMD_USER && data->cmd < CMD_192_RESERVED) ||
       (data->cmd >= CMD_USER_NR && data->cmd < CMD_LAST)) {

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
            "Got valid command No %d from %s client with %d bytes data ...\n",
            data->cmd, data->from, data->data_length);
        #endif

        ksnCorePacketData *rds = rd;

        // Process command
        rds->cmd = data->cmd;
        rds->from = data->from;
        rds->from_len = data->from_length;
        rds->data = memdup(data->from + data->from_length, data->data_length);
        rds->data_len = data->data_length;
        rds->l0_f = 1;

        // Execute L0 client command
        retval = ksnCommandCheck(ke->kc->kco, rds);
        if(retval) free(rds->data);  // \todo How to free the rds->data if not processed
    }
    // Wrong command
    else {

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
            "%s" "got wrong command No %d from %s client with %d bytes data, "
            "the command skipped ...%s\n",
            ANSI_RED,
            data->cmd, data->from, data->data_length,
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
 * @return
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
    ksn_printf(ke, MODULE, DEBUG_VV,
        "Got command No %d to \"%s\" L0 client from peer \"%s\" "
        "with %d bytes data\n",
        data->cmd, data->from, rd->from, data->data_length);
    #endif

    // Create L0 packet
    size_t out_data_len = sizeof(teoLNullCPacket) + rd->from_len +
            data->data_length;
    char *out_data = malloc(out_data_len);
    memset(out_data, 0, out_data_len);
    size_t packet_length = teoLNullPacketCreate(out_data, out_data_len,
            data->cmd, rd->from, data->from + data->from_length,
            data->data_length);

    // Send command to L0 client
    int *fd;
    size_t snd;
    size_t valueLength;
    fd = pblMapGet(ke->kl->map_n, data->from, data->from_length, &valueLength);
    if(fd != NULL) {

        if((snd = write(*fd, out_data, packet_length)) >= 0);

        #ifdef DEBUG_KSNET
        teoLNullCPacket *packet = (teoLNullCPacket *)out_data;
        //void *packet_data = packet->peer_name + packet->peer_name_length;
        ksn_printf(ke, MODULE, DEBUG_VV,
            "send %d bytes to \"%s\" L0 client: %d bytes data, "
            "from peer \"%s\"\n",
            (int)snd, data->from,
            packet->data_length, packet->peer_name);
        #endif
    }

    // The L0 client was disconnected
    else {

        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
            "%s" "the \"%s\" L0 client has not connected to the server%s\n",
            ANSI_RED, data->from, ANSI_NONE);
        #endif
    }

    free(out_data);

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
    
    if (tok->type == JSMN_STRING && 
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

        ALL_KEYS = USERID | CLIENTID | USERNAME | ACCESSTOKEN
    };
    const char *USERID_TAG = "userId";
    const char *CLIENTID_TAG = "clientId";
    const char *USERNAME_TAG = "username";
    const char *ACCESSTOKEN_TAG = "accessToken";
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
        
        // Find USERID tag
        find_tag(jp->userId, USERID, USERID_TAG)

        // Find CLIENTID tag
        else find_tag(jp->clientId, CLIENTID, CLIENTID_TAG)

        // Find USERNAME tag
        else find_tag(jp->username, USERNAME, USERNAME_TAG)

        // Find ACCESSTOKEN tag
        else find_tag(jp->accessToken, ACCESSTOKEN, ACCESSTOKEN_TAG)
                
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

    int retval = 0;
    ksnetEvMgrClass *ke = (ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke);

    // Parse request
    char *json_data_unesc = rd->data;
    json_param jp;
    json_parse(json_data_unesc, &jp);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG,
        "got %d bytes answer from %s authentication application,\n"
            "accessToken: %s,\n"
            "userId: %s,\n"
            "username: %s,\n"
            "clientId: %s,\n"
            "all data: %s\n",
        rd->data_len, TEO_AUTH, jp.accessToken, jp.userId, jp.username, 
        jp.clientId, rd->data
    );
    #endif

    ksnLNullClass *kl = ((ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke))->kl;
    int fd = ksnLNullClientIsConnected(kl, jp.accessToken);
    if(fd) {

        size_t vl;
        ksnLNullData* kld = pblMapGet(kl->map, &fd, sizeof(fd), &vl);
        if(kld != NULL) {

            // Rename client
            if(jp.userId != NULL && jp.clientId != NULL) {

                // Remove existing name
                if(kld->name != NULL) {
                    size_t vl;
                    pblMapRemoveFree(kl->map_n, kld->name, kld->name_length, &vl);
                    free(kld->name);
                }         
                // Set new name
                kld->name = ksnet_formatMessage("%s:%s", jp.userId, jp.clientId);        
                kld->name_length = strlen(kld->name) + 1;
                pblMapAdd(kl->map_n, kld->name, kld->name_length, &fd, sizeof(fd));
            }

            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG,
                "connection initialized, client name is: %s ...\n",
                kld->name);
            #endif

            // Send Connected event to all subscribers
            teoSScrSend(kev->kc->kco->ksscr, EV_K_L0_CONNECTED,
                    kld->name, kld->name_length, 0);

            // Add connection to L0 statistic
            kl->stat.visits++; // Increment number of visits

            // Send "new visit" event to all subscribers
            size_t vd_length = sizeof(ksnLNullSVisitsData) +
                    kld->name_length;
            ksnLNullSVisitsData *vd = malloc(vd_length);
            vd->visits = kl->stat.visits;
            memcpy(vd->client, kld->name, kld->name_length);
            teoSScrSend(kev->kc->kco->ksscr, EV_K_L0_NEW_VISIT,
                    vd, vd_length, 0);
            free(vd);        

            // Create & Send websocket allow answer message
            size_t snd;
            char *ALLOW = ksnet_formatMessage("{ \"name\": \"%s\" }", kld->name);
            size_t ALLOW_len = strlen(ALLOW) + 1;
            // Create L0 packet
            size_t out_data_len = sizeof(teoLNullCPacket) + rd->from_len +
                    ALLOW_len;
            char *out_data = malloc(out_data_len);
            memset(out_data, 0, out_data_len);
            size_t packet_length = teoLNullPacketCreate(out_data, 
                    out_data_len, CMD_L0_AUTH, rd->from, ALLOW, ALLOW_len);
            // Send websocket allow message
            if((snd = write(fd, out_data, packet_length)) >= 0);
            free(out_data);
        }

        // Free tags
        if(jp.accessToken != NULL) free(jp.accessToken);
        if(jp.clientId != NULL) free(jp.clientId);
        if(jp.userId != NULL) free(jp.userId);
        if(jp.username != NULL) free(jp.username);
    }        

    return retval;
}

/**
 * Create list of clients
 *
 * @param kl
 * @return
 */
teonet_client_data_ar *ksnLNullClientsList(ksnLNullClass *kl) {

    teonet_client_data_ar *data_ar = NULL;

    if(kl != NULL && kev->ksn_cfg.l0_allow_f && kl->fd) {

        uint32_t length = pblMapSize(kl->map);
        data_ar = malloc(sizeof(teonet_client_data_ar) +
                length * sizeof(data_ar->client_data[0]));
        //data_ar->length = length;
        int i = 0;

        // Create clients list
        PblIterator *it = pblMapIteratorReverseNew(kl->map);
        if(it != NULL) {
            while(pblIteratorHasPrevious(it) > 0) {
                void *entry = pblIteratorPrevious(it);
                //int *fd = (int *) pblMapEntryKey(entry);
                ksnLNullData *data = pblMapEntryValue(entry);
                if(data != NULL) {
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

#undef kev
