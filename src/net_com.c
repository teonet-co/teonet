/**
 * File:   net_com.c
 * Author: Kirill Scherba
 *
 * Created on April 29, 2015, 7:57 PM
 *
 * KSNet Network command processing module
 */

#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "net_split.h"

/**
 * KSNet CMD_PEER command data
 */
typedef struct ksnCmdPeerData {

    char *name;     ///< Peer name
    char *addr;     ///< Peer IP address
    uint32_t port;  ///< Peer port

} ksnCmdPeerData;

// Local functions
int cmd_echo_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_echo_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_connect_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_connect_r_cb(ksnCommandClass *kco, ksnCorePacketData *rd);

/**
 * Initialize ksnet command class
 *
 * @return
 */
ksnCommandClass *ksnCommandInit(void *kc) {

    ksnCommandClass *kco = malloc(sizeof(ksnCommandClass));
    kco->kc = kc;
    kco->ks = ksnSplitInit(kco);

    return kco;
}

/**
 * Destroy ksnet command class
 * @param kco
 */
void ksnCommandDestroy(ksnCommandClass *kco) {

    ksnSplitDestroy(kco->ks);
    free(kco);
}

/**
 * Check and process command
 *
 * @return True if command processed
 */
int ksnCommandCheck(ksnCommandClass *kco, ksnCorePacketData *rd) {

    int processed = 0;

    switch(rd->cmd) {

        case CMD_NONE:
            //printf("CMD_NONE\n");
            processed = 1;
            break;

        case CMD_ECHO:
            //printf("CMD_ECHO\n");
            processed = cmd_echo_cb(kco, rd);
            break;

        case CMD_ECHO_ANSWER:
            //printf("CMD_ECHO_ANSWER\n");
            processed = cmd_echo_answer_cb(kco, rd);
            break;

        case CMD_CONNECT_R:
            processed = cmd_connect_r_cb(kco, rd);
            break;

        case CMD_CONNECT:
            processed = cmd_connect_cb(kco, rd);
            break;

        case CMD_DISCONNECTED:
            processed = cmd_disconnected_cb(kco, rd);
            break;

        #if M_ENAMBE_VPN
        case CMD_VPN:
            processed = cmd_vpn_cb(
                ((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke)->kvpn,
                rd->from,
                rd->data,
                rd->data_len
            );
            break;
        #endif

        case CMD_SPLIT:
            {
                ksnCorePacketData *rds = ksnSplitCombine(kco->ks, rd);
                if(rds != NULL) {
                    processed = ksnCommandCheck(kco, rds);
                    if(!processed) {
                        // Send event callback
                        ksnetEvMgrClass *ke = ((ksnCoreClass*)kco->kc)->ke;
                        if(ke->event_cb != NULL)
                            ke->event_cb(ke, EV_K_RECEIVED, (void*)rds, sizeof(rds), NULL);

                        processed = 1;
                    }
                    ksnSplitFreRds(kco->ks, rds);
                }
                else processed = 1;
            }
            break;

//        case CMD_TUN:
//            processed = cmd_tun_cb(
//                ((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke)->ktun,
//                rd
//            );
//            break;

        default:
            break;
    }

    return processed;
}

/**
 * Send ECHO command to peer
 *
 * @param kco
 * @param to
 * @param data
 * @param data_len
 * @return
 */
int ksnCommandSendCmdEcho(ksnCommandClass *kco, char *to, void *data,
                          size_t data_len) {

    double ct = ksnetEvMgrGetTime(((ksnCoreClass *) kco->kc)->ke);;
    size_t data_t_len = data_len + sizeof(double);
    void *data_t = malloc(data_t_len);

    memcpy(data_t, data, data_len);
    *((double*)(data_t + data_len)) = ct;

    ksnet_arp_data * arp = ksnCoreSendCmdto(kco->kc, to, CMD_ECHO, data_t,
                                            data_t_len);
    if(arp != NULL) arp->last_triptime_send = ct;
    free(data_t);

    return arp != NULL;
}

/**
 * Send CONNECTED command to peer
 *
 * @param kco Pointer to ksnCommandClass
 * @param to Send command to peer name
 * @param name Name of peer to connect
 * @param addr IP address of peer to connect
 * @param port Port of peer to connect
 * @return
 */
int ksnCommandSendCmdConnect(ksnCommandClass *kco, char *to, char *name,
        char *addr, uint32_t port) {

    // Create command data
    size_t ptr = 0;
    char data[KSN_BUFFER_DB_SIZE];
    strcpy(data, name); ptr = strlen(name) + 1;
    strcpy(data + ptr, addr); ptr += strlen(addr) + 1;
    *((uint32_t *)(data + ptr)) = port; ptr += sizeof(uint32_t);

    return ksnCoreSendCmdto(kco->kc, to, CMD_CONNECT, data, ptr) != NULL;
}

/**
 * Process CMD_ECHO
 *
 * @param from
 * @param data
 * @param data_length
 * @return
 */
int cmd_echo_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    // Send echo answer command
    ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_ECHO_ANSWER,
                  rd->data, rd->data_len);

    return 1; // Command processed
}

/**
 * Process CMD_ECHO_ANSWER
 *
 * @param from
 * @param data
 * @param data_length
 * @return
 */
int cmd_echo_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #define ke ((ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke))

    double
        time_got = ksnetEvMgrGetTime(ke),
        time_send = *(double*)(rd->data + rd->data_len - sizeof(double)),
        triptime = (time_got - time_send) * 1000.0;

    // Set trip time and last tip time got
    rd->arp->last_triptime_got = time_got;
    rd->arp->last_triptime = triptime;
    rd->arp->triptime = (rd->arp->triptime + triptime) / 2.0;

    // Ping answer
    if(!strcmp(rd->data, PING)) {
        // Show command message
        #ifdef DEBUG_KSNET
        ksnet_printf(& ke->ksn_cfg, DEBUG,
            "%d bytes from %s: cmd=cmd_echo ttl=57 time=%.3f ms\n",
            rd->data_len, // command data length
            rd->from,     // from
            triptime
        );
        #endif
    }

    // Trim time answer
    else if(!strcmp(rd->data, TRIPTIME)) {
        // ...
    }

    // Monitor answer
    else if(!strcmp(rd->data, MONITOR)) {
        // Show command message
        #ifdef DEBUG_KSNET
        ksnet_printf(& ke->ksn_cfg, DEBUG_VV,
            "%d bytes from %s: cmd=cmd_echo ttl=57 time=%.3f ms\n",
            rd->data_len,   // command data length
            rd->from,       // from
            triptime
        );
        #endif

        // Set monitor time
        ksnet_arp_data *arp_data = ksnetArpGet(ke->kc->ka, rd->from);
        arp_data->monitor_time = time_got - time_send;
        ksnetArpAdd(ke->kc->ka, rd->from, arp_data);
    }

    else {

        // Show command message
        #ifdef DEBUG_KSNET
        ksnet_printf(& ke->ksn_cfg, DEBUG,
            "Net command module: "
            "received Echo answer command => from '%s': %d byte data: %s, %.3f ms\n",
            rd->from,        // from
            rd->data_len,    // command data length
            rd->data,        // commands data
            triptime
        );
        #endif
    }

    return 1;

    #undef ke
}

/**
 * Send connect command command
 *
 * @param ka
 * @param peer_name
 * @param arp_data
 * @param data
 */
int send_cmd_connect_cb(ksnetArpClass *ka, char *peer_name,
                        ksnet_arp_data *arp_data, void *data) {

    #define rd ((ksnCorePacketData*)data)

    if(strcmp(peer_name, rd->from)) {
        ksnCommandSendCmdConnect( ((ksnetEvMgrClass*) ka->ke)->kc->kco,
                peer_name, rd->from, rd->addr, rd->port);
    }

    return 0;
    #undef rd
}

/**
 * Process CMD_CONNECT_R. A Peer want connect to r-host
 *
 * @param kco
 * @param rd
 * @return
 */
int cmd_connect_r_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    // Replay to address we got from peer
    ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_NONE,
                  NULL_STR, 1);

    // Parse command data
    size_t i, ptr = 0;
    ksnCorePacketData lrd;
    uint8_t *num_ip = rd->data; ptr += sizeof(uint8_t); // Number of IPs
    lrd.port = *((uint32_t*)(rd->data + rd->data_len - sizeof(uint32_t))); // Port number
    lrd.from = rd->from;
    for(i = 0; i <= *num_ip; i++ ) {

        if(!i) lrd.addr = (char*)localhost;
        else {
            lrd.addr = rd->data + ptr; ptr += strlen(lrd.addr) + 1;
        }

        // Send local IP address and port to child
        ksnetArpGetAll( ((ksnCoreClass*)kco->kc)->ka, send_cmd_connect_cb, &lrd);
    }

    // Send peer address to child
    ksnetArpGetAll( ((ksnCoreClass*)kco->kc)->ka, send_cmd_connect_cb, rd);

    return 1;
}

/**
 * Process CMD_CONNECT. A peer send his child to us
 *
 * @param kco
 * @param rd
 * @return
 */
int cmd_connect_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    ksnCmdPeerData pd;

    // Parse command data
    size_t ptr = 0;
    pd.name = rd->data; ptr += strlen(pd.name) + 1;
    pd.addr = rd->data + ptr; ptr += strlen(pd.addr) + 1;
    pd.port = *((uint32_t *)(rd->data + ptr));

    #ifdef DEBUG_KSNET
        ksnet_printf(
            &((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke)->ksn_cfg,
            DEBUG_VV,
            "cmd_connect_cb: %s %s:%d\n", pd.name, pd.addr, pd.port);
    #endif

    // Check ARP
    if(ksnetArpGet(((ksnCoreClass*)kco->kc)->ka, pd.name) == NULL) {

        ksnCoreSendto(kco->kc, pd.addr, pd.port, CMD_NONE, NULL_STR, 1);
    }

    return 1;
}

/**
 * Process CMD_DISCONNECTED. A peer send disconnect command
 *
 * @param kco
 * @param rd
 * @return
 */
int cmd_disconnected_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #define kev ((ksnetEvMgrClass*) ((ksnCoreClass *) kco->kc)->ke)

    // Check r-host disconnected
    if(kev->ksn_cfg.r_host_name[0] && !strcmp(kev->ksn_cfg.r_host_name,
                                              rd->from)) {

        kev->ksn_cfg.r_host_name[0] = '\0';
    }

    // Remove from ARP Table
    ksnetArpRemove(((ksnCoreClass*)kco->kc)->ka, rd->from);

    // Send event callback
    if(kev->event_cb != NULL)
        kev->event_cb(kev, EV_K_DISCONNECTED, (void*)rd, sizeof(*rd), NULL);

    return 1;

    #undef kev
}
