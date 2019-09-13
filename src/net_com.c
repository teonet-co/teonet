/**
 * File:   net_com.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 29, 2015, 7:57 PM
 *
 * KSNet Network command processing module
 */

#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "net_split.h"
#include "net_recon.h"
#include "utils/rlutil.h"
#include "utils/teo_memory.h"
#include "modules/subscribe.h"
#if TRUDP_VERSION == 2
#include "trudp_stat.h"
#endif

// Local functions
static int cmd_echo_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_echo_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_connect_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_connect_r_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_stream_cb(ksnStreamClass *ks, ksnCorePacketData *rd);
int cmd_l0_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd);
int cmd_l0_to_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd);
static int cmd_peers_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_peers_num_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_resend_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_reconnect_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_reconnect_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_split_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_l0_clients_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_l0_clients_n_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_subscribe_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_l0_stat_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_host_info_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_host_info_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_reset_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_l0_info_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_trudp_info_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_l0_check_cb(ksnCommandClass *kco, ksnCorePacketData *rd);

// Constant
const char *JSON = "JSON";
const char *BINARY = "BINARY";

#define MODULE _ANSI_LIGHTBLUE "net_command" _ANSI_NONE

/**
 * Initialize ksnet command class
 *
 * @return
 */
ksnCommandClass *ksnCommandInit(void *kc) {

    ksnCommandClass *kco = teo_malloc(sizeof(ksnCommandClass));
    kco->kc = kc;
    kco->ks = ksnSplitInit(kco);
    kco->kr = ksnReconnectInit(kco);
    kco->ksscr = teoSScrInit(((ksnCoreClass *)kc)->ke);

    return kco;
}

/**
 * Destroy ksnet command class
 * @param kco
 */
void ksnCommandDestroy(ksnCommandClass *kco) {

    teoSScrDestroy(kco->ksscr); // Destroy subscribe class
    ksnSplitDestroy(kco->ks); // Destroy split class
    ((ksnReconnectClass*)kco->kr)->destroy(kco->kr); // Destroy reconnect class
    free(kco);
}

/**
 * Check and process command
 *
 * @return True if command processed
 */
int ksnCommandCheck(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #define kev ((ksnetEvMgrClass*) ((ksnCoreClass *) kco->kc)->ke)

    int processed = 0;

    switch(rd->cmd) {

        case CMD_NONE:
            processed = 1;
            break;

        case CMD_RESET:
            processed = cmd_reset_cb(kco, rd);
            break;

        case CMD_ECHO:
            processed = cmd_echo_cb(kco, rd);
            break;

        case CMD_ECHO_ANSWER:
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
            processed = cmd_vpn_cb(kev->kvpn, rd->from, rd->data, rd->data_len);
            break;
        #endif

        case CMD_SPLIT:
            processed = cmd_split_cb(kco, rd);
            break;

        #ifdef M_ENAMBE_TUN
        case CMD_TUN:
            processed = cmd_tun_cb(kev->ktun, rd);
            break;
        #endif

        #ifdef M_ENAMBE_STREAM
        case CMD_STREAM:
            processed = cmd_stream_cb(kev->ks, rd);
            break;
        #endif

        #ifdef M_ENAMBE_L0s
        case CMD_L0:
            processed = cmd_l0_cb(kev, rd);
            break;
        #endif

        #ifdef M_ENAMBE_L0s
        case CMD_L0_TO:
            processed = cmd_l0_to_cb(kev, rd);
            break;
        #endif

        case CMD_PEERS:
            processed = cmd_peers_cb(kco, rd);
            break;

        case CMD_GET_NUM_PEERS:
            processed = cmd_peers_num_cb(kco, rd);
            break;

        case CMD_RESEND:
            processed = cmd_resend_cb(kco, rd);
            break;

        case CMD_RECONNECT:
            processed = cmd_reconnect_cb(kco, rd);
            break;

        case CMD_RECONNECT_ANSWER:
            processed = cmd_reconnect_answer_cb(kco, rd);
            break;

        case CMD_L0_CLIENTS:
            processed = cmd_l0_clients_cb(kco, rd);
            break;

        case CMD_L0_CLIENTS_N:
            processed = cmd_l0_clients_n_cb(kco, rd);
            break;

        case CMD_L0_STAT:
            processed = cmd_l0_stat_cb(kco, rd);
            break;

        case CMD_L0_INFO:
            processed = cmd_l0_info_cb(kco, rd);
            break;

        case CMD_HOST_INFO:
            processed = cmd_host_info_cb(kco, rd);
            break;

        case CMD_HOST_INFO_ANSWER:
            processed = cmd_host_info_answer_cb(kco, rd);
            break;
            
        case CMD_TRUDP_INFO:
            processed = cmd_trudp_info_cb(kco, rd);
            break;

        case CMD_SUBSCRIBE:
        case CMD_UNSUBSCRIBE:
        case CMD_SUBSCRIBE_ANSWER:
        case CMD_SUBSCRIBE_RND:
            processed = cmd_subscribe_cb(kco, rd);
            break;

        case CMD_L0_AUTH: // CMD_USER + 1:
            processed = cmd_l0_check_cb(kco, rd);
            break;

        default:
            break;
    }

    return processed;

    #undef kev
}

/**
 * Create ECHO command buffer
 *
 * @param kco Pointer to ksnCommandClass
 * @param data Echo data
 * @param data_len Echo data length
 * @param data_t_len Pointer to hold ECHO buffer size
 *
 * @return Pointer to ECHO command buffer. SHould be free after use.
 */
void *ksnCommandEchoBuffer(ksnCommandClass *kco, void *data, size_t data_len,
        size_t *data_t_len) {

    double ct = ksnetEvMgrGetTime(((ksnCoreClass *) kco->kc)->ke);;
    *data_t_len = data_len + sizeof(double);
    void *data_t = malloc(*data_t_len);

    memcpy(data_t, data, data_len);
    *((double*)(data_t + data_len)) = ct;

    return data_t;
}

/**
 * Send ECHO command to peer
 *
 * @param kco Pointer to ksnCommandClass
 * @param to Send command to peer name
 * @param data Echo data
 * @param data_len Echo data length
 *
 * @return True at success
 */
int ksnCommandSendCmdEcho(ksnCommandClass *kco, char *to, void *data,
                          size_t data_len) {

    size_t data_t_len;
    void *data_t = ksnCommandEchoBuffer(kco, data, data_len, &data_t_len);

    ksnet_arp_data * arp = ksnCoreSendCmdto(kco->kc, to, CMD_ECHO, data_t,
                                            data_t_len);

    if(arp != NULL) arp->last_triptime_send = ksnetEvMgrGetTime(
            ((ksnCoreClass *) kco->kc)->ke);

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
    strncpy(data, name, KSN_BUFFER_DB_SIZE); ptr = strlen(name) + 1;
    strncpy(data + ptr, addr, KSN_BUFFER_DB_SIZE - ptr); ptr += strlen(addr) + 1;
    *((uint32_t *)(data + ptr)) = port; ptr += sizeof(uint32_t);

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "send CMD_CONNECT (cmd = 5) command to peer %s\n",
            to);
    #endif

    return ksnCoreSendCmdto(kco->kc, to, CMD_CONNECT, data, ptr) != NULL;
}

/**
 * Send CONNECTED command to peer by address
 *
 * @param kco Pointer to ksnCommandClass
 * @param to_addr Send command to IP address 
 * @param to_port Send command to Port 
 * @param name Name of peer to connect
 * @param addr IP address of peer to connect
 * @param port Port of peer to connect
 * @return
 */
int ksnCommandSendCmdConnectA(ksnCommandClass *kco, char *to_addr, uint32_t to_port, 
        char *name, char *addr, uint32_t port) {

    // Create command data
    size_t ptr = 0;
    char data[KSN_BUFFER_DB_SIZE];
    strncpy(data, name, KSN_BUFFER_DB_SIZE); ptr = strlen(name) + 1;
    strncpy(data + ptr, addr, KSN_BUFFER_DB_SIZE - ptr); ptr += strlen(addr) + 1;
    *((uint32_t *)(data + ptr)) = port; ptr += sizeof(uint32_t);
    
    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "send CMD_CONNECT (cmd = 5) command to peer by address %s:%d\n",
            to_addr, to_port);
    #endif

    return ksnCoreSendto(kco->kc, to_addr, to_port, CMD_CONNECT, data, ptr);
}

/**
 * Process CMD_RESET command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_reset_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    int processed = 0;

    // Hard reset
    if(rd->data_len >= 1 &&
            (((char*)rd->data)[0] == '\1' || ((char*)rd->data)[0] == '1')) {

        // Execute restart of this application
        kill(getpid(),SIGUSR2);

        processed = 1;
    }

    // Soft reset
    else {
        // Do nothing, just send it to Application level
    }

    return processed; // Command processed
}

/**
 * Process CMD_ECHO command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_echo_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_ECHO (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Send ECHO to L0 user
    if(rd->l0_f)
        ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                rd->addr, rd->port, rd->from, rd->from_len, CMD_ECHO_ANSWER,
                rd->data, rd->data_len);

    // Send echo answer command to peer
    else
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_ECHO_ANSWER,
                rd->data, rd->data_len);

    return 1; // Command processed
}

/**
 * Process CMD_PEERS command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_peers_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_PEERS (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get peers data
    ksnet_arp_data_ar *peers_data = ksnetArpShowData(((ksnCoreClass*)kco->kc)->ka);
    size_t peers_data_length = ksnetArpShowDataLength(peers_data);

    // Get type of request: 0 - binary; 1 - JSON
    const int data_type = rd->data_len && !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

    // Convert data to JSON format
    if(data_type == 1) {

        size_t peers_json_len;
        char *peers_json = ksnetArpShowDataJson(peers_data, &peers_json_len);

        free(peers_data);
        peers_data = (ksnet_arp_data_ar *)peers_json;
        peers_data_length = peers_json_len;
    }

    // Send PEERS_ANSWER to L0 user
    if(rd->l0_f)
        ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                rd->addr, rd->port, rd->from, rd->from_len, CMD_PEERS_ANSWER,
                peers_data, peers_data_length);

    // Send PEERS_ANSWER to peer
    else
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_PEERS_ANSWER,
                peers_data, peers_data_length);

    free(peers_data);

    return 1; // Command processed
}

/**
 * Process CMD_GET_NUM_PEERS command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_peers_num_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_GET_NUM_PEERS (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    void *peers_data;
    size_t peers_data_length = 0;

    // Get type of request: 0 - binary; 1 - JSON
    const int data_type = rd->data_len && !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

    // Get peers number data
    uint32_t peers_number = ksnetArpSize(((ksnCoreClass*)kco->kc)->ka); // Get peers number

    // JSON data type
    if(data_type == 1) {

        char *json_str = ksnet_formatMessage(
            "{ \"numPeers\": %d }",
            peers_number
        );

        peers_data = json_str;
        peers_data_length = strlen(json_str) + 1;
    }

    // BINARY data type
    else {

        peers_data_length = sizeof(peers_number);
        peers_data = memdup(&peers_number, peers_data_length);
    }

    // Send PEERS_ANSWER to L0 user
    if(rd->l0_f)
        ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                rd->addr, rd->port, rd->from, rd->from_len,
                CMD_GET_NUM_PEERS_ANSWER,
                peers_data, peers_data_length);

    // Send PEERS_ANSWER to peer
    else
        ksnCoreSendto(kco->kc, rd->addr, rd->port,
                CMD_GET_NUM_PEERS_ANSWER,
                peers_data, peers_data_length);

    free(peers_data);

    return 1; // Command processed
}

/**
 * Process CMD_L0_CLIENTS command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_l0_clients_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_L0_CLIENTS (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get l0 clients data
    teonet_client_data_ar *client_data =
        ksnLNullClientsList(((ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke))->kl);

    if(client_data != NULL) {

        size_t client_data_length = ksnLNullClientsListLength(client_data);

        // Send CMD_L0_CLIENTS_ANSWER to L0 user
        if(rd->l0_f)
            ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                    rd->addr, rd->port, rd->from, rd->from_len,
                    CMD_L0_CLIENTS_ANSWER, client_data, client_data_length);

        // Send PEERS_ANSWER to peer
        else
            ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_L0_CLIENTS_ANSWER,
                    client_data, client_data_length);

        free(client_data);
    }

    return 1; // Command processed
}

/**
 * Process CMD_L0_CLIENTS_N command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_l0_clients_n_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_L0_CLIENTS_N (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get l0 clients data
    teonet_client_data_ar *client_data =
        ksnLNullClientsList(((ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke))->kl);

    uint32_t clients_number = client_data == NULL ? 0 : client_data->length;

    char *json_str = NULL;
    void *data_out = &clients_number;
    size_t data_out_len = sizeof(clients_number);
    if(rd->data_len && !strncmp(rd->data, JSON, rd->data_len)) {

        json_str = ksnet_sformatMessage(json_str,
            "{ \"numClients\": %d }", clients_number
        );
        data_out = json_str;
        data_out_len = strlen(json_str) + 1;
    }

    // Send CMD_L0_CLIENTS_ANSWER to L0 user
    if(rd->l0_f)
        ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                rd->addr, rd->port, rd->from, rd->from_len,
                CMD_L0_CLIENTS_N_ANSWER, data_out, data_out_len);

    // Send PEERS_ANSWER to peer
    else
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_L0_CLIENTS_N_ANSWER,
                data_out, data_out_len);

    if(client_data != NULL) free(client_data);
    if(json_str != NULL) free(json_str);

    return 1; // Command processed
}

/**
 * Process CMD_L0_INFO command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_l0_info_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_L0_INFO (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    l0_info_data *info_d = NULL;
    size_t info_d_len = 0;

    // Get L0 info
    ksnetEvMgrClass *ke = (ksnetEvMgrClass*)((ksnCoreClass*)(kco->kc))->ke;
    if(ke->kl != NULL) {

        if(ke->ksn_cfg.l0_allow_f) {

            if(ke->ksn_cfg.l0_tcp_ip_remote[0]) {

                size_t l0_tcp_ip_remote_len =  strlen(ke->ksn_cfg.l0_tcp_ip_remote) + 1;
                info_d_len = sizeof(l0_info_data) + l0_tcp_ip_remote_len;
                info_d = malloc(info_d_len);

                memcpy(info_d->l0_tcp_ip_remote, ke->ksn_cfg.l0_tcp_ip_remote, l0_tcp_ip_remote_len);
                info_d->l0_tcp_port = ke->ksn_cfg.l0_tcp_port;
            }
        }
    }

    // Send CMD_L0_INFO_ANSWER to L0 user
    if(rd->l0_f)
        ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                rd->addr, rd->port, rd->from, rd->from_len,
                CMD_L0_INFO_ANSWER,
                info_d, info_d_len);

    // Send L0_STAT_ANSWER to peer
    else
        ksnCoreSendto(kco->kc, rd->addr, rd->port,
                CMD_L0_INFO_ANSWER,
                    info_d, info_d_len);

    if(info_d != NULL) free(info_d);

    return 1; // Command processed
}

/**
 * Process CMD_L0_STAT command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_l0_stat_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_L0_STAT (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get l0 clients statistic
    ksnLNullSStat *stat_data =
        ksnLNullStat(((ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke))->kl);

    if(stat_data != NULL) {

        // Get type of request: 0 - binary; 1 - JSON
        const int data_type = rd->data_len &&
                              !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

        size_t l0_stat_data_len = sizeof(ksnLNullSStat);
        void *l0_stat_data = stat_data;
        char *json_str = NULL;

        // JSON data type
        if(data_type == 1) {

            json_str = ksnet_formatMessage(
                "{ \"visits\": %d }",
                stat_data->visits
            );

            l0_stat_data = json_str;
            l0_stat_data_len = strlen(json_str) + 1;
        }

        // Send L0_STAT_ANSWER to L0 user
        if(rd->l0_f)
            ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                    rd->addr, rd->port, rd->from, rd->from_len,
                    CMD_L0_STAT_ANSWER,
                    l0_stat_data, l0_stat_data_len);

        // Send L0_STAT_ANSWER to peer
        else
            ksnCoreSendto(kco->kc, rd->addr, rd->port,
                    CMD_L0_STAT_ANSWER,
                    l0_stat_data, l0_stat_data_len);

        // Free json string data
        if(json_str != NULL) free(json_str);
    }

    return 1; // Command processed
}


/**
 * Process CMD_HOST_INFO_ANSWER
 *
 * @param kco
 * @param rd
 * @return
 */
static int cmd_host_info_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {
    
    ksnetEvMgrClass *ke = EVENT_MANAGER_CLASS(kco);
    ksnetArpClass *arp_class = ((ksnetArpClass*)((ksnCoreClass*)kco->kc)->ka);

    const int not_json = rd->data_len && ((char*)rd->data)[0] != '{' && ((char*)rd->data)[rd->data_len-1] != '}';
    
    int retval = 0;
    if (not_json) {
        ksnet_arp_data_ext *arp_cque =  ksnetArpGet(arp_class, rd->from);
        ksnCQueExec(ke->kq, arp_cque->cque_id_peer_type);
    
        if(!rd->arp->type) {
            
            host_info_data *hid = (host_info_data *)rd->data;
            int i;
            size_t ptr     = strlen(hid->string_ar) + 1;
            char *type_str = strdup(null_str);
            for (i = 1; i < hid->string_ar_num; i++) {
                type_str = ksnet_sformatMessage(type_str, "%s%s\"%s\"",
                    type_str, i > 1 ? ", " : "", hid->string_ar + ptr);

                ptr += strlen(hid->string_ar + ptr) + 1;
            }

            // Add type to arp-table
            rd->arp->type = strdup(type_str);
            free(type_str);

            // Send event callback
            if(ke->event_cb != NULL)
                ke->event_cb(ke, EV_K_CONNECTED, (void*)rd, sizeof(*rd), NULL);

            // Send event to subscribers
            teoSScrSend(kco->ksscr, EV_K_CONNECTED, rd->from, rd->from_len, 0);

            #ifdef DEBUG_KSNET
            ksn_printf(ke,
                MODULE, DEBUG_VV,
                "process CMD_HOST_INFO_ANSWER (cmd = %u) command, from %s (%s:%d), arp-addr %s:%d, type: %s\n",
                rd->cmd, rd->from, rd->addr, rd->port, rd->arp->data.addr, rd->arp->data.port, rd->arp->type);
            #endif
            retval = 1;
        }
    }

    return retval; // Command send to user level
}

/**
 * Process CMD_HOST_INFO
 *
 * @param kco
 * @param rd
 * @return
 */
static int cmd_host_info_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_HOST_INFO (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get host info
    size_t hid_len;
    ksnetEvMgrClass *ke = (ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke);
    host_info_data *hid = teoGetHostInfo(ke, &hid_len);

    if(hid != NULL) {
        // Get type of request: 0 - binary; 1 - JSON
        const int data_type = rd->data_len &&
                              !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

        void *data_out = hid;
        char *json_str = NULL;
        size_t data_out_len = hid_len;

        // JSON data type
        if(data_type == 1) {

            // Combine types
            int i;
            size_t ptr = strlen(hid->string_ar) + 1;
            char *type_str = strdup(null_str);
            for(i = 1; i < hid->string_ar_num; i++) {

                type_str = ksnet_sformatMessage(type_str, "%s%s\"%s\"",
                        type_str, i > 1 ? ", " : "", hid->string_ar + ptr);
                ptr += strlen(hid->string_ar + ptr) + 1;
            }
            char *app_version = hid->string_ar + ptr; // Last element is app_version
            ptr += strlen(hid->string_ar + ptr) + 1;
            json_str = ksnet_formatMessage(
                "{ "
                    "\"name\": \"%s\", "
                    "\"type\": [ %s ], "
                    "\"appType\": [ %s ], "
                    "\"version\": \"%d.%d.%d\", "
                    "\"app_version\": \"%s\", "
                    "\"appVersion\": \"%s\""
                " }",
                hid->string_ar, // Name
                type_str, // App type and services
                type_str, // App type and services (copy to another json field)
                hid->ver[0], hid->ver[1], hid->ver[2], // Version
                app_version, // Application version
                app_version  // Application version (copy to another json field)
            );
            free(type_str);

            data_out = json_str;
            data_out_len = strlen(json_str) + 1;
        }

        // Send PEERS_ANSWER to L0 user
        if(rd->l0_f) {
            ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                    rd->addr, rd->port, rd->from, rd->from_len,
                    CMD_HOST_INFO_ANSWER,
                    data_out, data_out_len);

        // Send HOST_INFO_ANSWER to peer
        } else {
            ksnCoreSendto(kco->kc, rd->addr, rd->port,
                    CMD_HOST_INFO_ANSWER,
                    data_out, data_out_len);
	}
        // Free json string data
        if(json_str != NULL) free(json_str);
        free(hid);
    }

    return 1; // Command processed
}

/**
 * Process CMD_TRUDP_INFO
 *
 * @param kco
 * @param rd
 * @return
 */
static int cmd_trudp_info_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_TRUDP_INFO (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    ksnetEvMgrClass *ke = (ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke);

    // Get type of request: 0 - binary; 1 - JSON
    const int data_type = rd->data_len &&
                          !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

    // Get TR-UDP info
    size_t data_out_len;
    #if TRUDP_VERSION == 1
    void *data_out = ksnTRUDPstatGet(ke->kc->ku, data_type, &data_out_len);
    #elif TRUDP_VERSION == 2
    void *data_out = trudpStatGet(ke->kc->ku, data_type, &data_out_len);
    #endif

    // Send TRUDP_INFO_ANSWER to L0 user
    if(rd->l0_f)
        ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
                rd->addr, rd->port, rd->from, rd->from_len,
                CMD_TRUDP_INFO_ANSWER,
                data_out, data_out_len);

    // Send TRUDP_INFO_ANSWER to peer
    else
        ksnCoreSendto(kco->kc, rd->addr, rd->port,
                CMD_TRUDP_INFO_ANSWER,
                data_out, data_out_len);

    if(data_out != NULL) free(data_out);

    return 1; // Command processed
}

/**
 * Process CMD_RESEND command
 *
 * The RESEND command sent by sender to peer if sender does not connected to
 * peer. This function resend command to peer and send CONNECT command to sender
 * and peer to connect it direct.
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_resend_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_RESEND (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Parse CMD_RESEND data
    size_t ptr = 0;
    char *to = rd->data + ptr; ptr += strlen(rd->data) + 1;
    uint8_t cmd = *((uint8_t*)(rd->data + ptr)); ptr += sizeof(uint8_t);
    void *data = rd->data + ptr;
    const size_t data_len = rd->data_len - ptr;

    // Resend command to peer
    ksnCoreSendCmdto(kco->kc, to, cmd, data, data_len);

    // If we resend command from sender, than sender don't know about the peer,
    // try send connect command to peer to direct connect sender with peer
    ksnet_arp_data *arp;
    if((arp = (ksnet_arp_data *)ksnetArpGet(((ksnCoreClass*)kco->kc)->ka, to)) != NULL) {

        // Send connect command request to peer
        ksnCommandSendCmdConnect(kco, to, rd->from, rd->addr, rd->port);

        // Send connect command request to sender
        ksnCommandSendCmdConnect(kco, rd->from, to, arp->addr, arp->port);
    }

    return 1; // Command processed
}


/**
 * Process CMD_RECONNECT command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
inline int cmd_reconnect_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_RECONNECT (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    return ((ksnReconnectClass*)kco->kr)->process(kco->kr, rd); // Process reconnect command
}

/**
 * Process CMD_RECONNECT_ANSWER command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
inline static int cmd_reconnect_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_RECONNECT_ANSWER (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    return ((ksnReconnectClass*)kco->kr)->processAnswer(kco->kr, rd); // Process reconnect answer command
}

/**
 * Process CMD_ECHO_ANSWER command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_echo_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_ECHO_ANSWER (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    #define ke ((ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke))

    double
        time_got = ksnetEvMgrGetTime(ke),
        time_send = *(double*)(rd->data + rd->data_len - sizeof(double)),
        triptime = (time_got - time_send) * 1000.0;

    // Set trip time and last tip time got
    rd->arp->data.last_triptime_got = time_got;
    rd->arp->data.last_triptime = triptime;
    rd->arp->data.triptime = (rd->arp->data.triptime + triptime) / 2.0;

    // Ping answer
    if(!strcmp(rd->data, PING)) {

        // Show command message
        //#ifdef DEBUG_KSNET
        ksnet_printf(& ke->ksn_cfg, DISPLAY_M,
            "%d bytes from %s: cmd=cmd_echo ttl=57 time=%.3f ms\n",
            (int)rd->data_len, // command data length
            rd->from,          // from
            triptime           // triptime
        );
        //#endif
    }

    // Trim time answer
    else if(!strcmp(rd->data, TRIPTIME)) {
        // ...
    }

    // Monitor answer
    else if(!strcmp(rd->data, MONITOR)) {

        // Show command message
        //#ifdef DEBUG_KSNET
        ksnet_printf(& ke->ksn_cfg, DISPLAY_M,
            "%d bytes from %s: cmd=cmd_echo ttl=57 time=%.3f ms\n",
            (int)rd->data_len,   // command data length
            rd->from,            // from
            triptime             // triptime
        );
        //#endif

        // Set monitor time
        ksnet_arp_data_ext *arp = ksnetArpGet(ke->kc->ka, rd->from);
        arp->data.monitor_time = time_got - time_send;
        ksnetArpAdd(ke->kc->ka, rd->from, arp);
    }

    else {

        // Show command message
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV,
            "got echo answer from \"%s\", %d byte data: \"%.*s\", %.3f ms\n",
            rd->from,        // from
            rd->data_len,    // command data length
            rd->data_len == 8 ? 0 : rd->data_len,    // command data length
            rd->data,        // commands data
            triptime         // trip time
        );
        #endif
    }

    return 0;

    #undef ke
}

/**
 * Send connect command
 *
 * @param ka Pointer to ksnetArpClass
 * @param peer_name Peer name
 * @param arp_data Pointer to ARP data ksnet_arp_data
 * @param data Pointer to ksnCorePacketData
 */
static int send_cmd_connect_cb(ksnetArpClass *ka, char *peer_name,
                        ksnet_arp_data_ext *arp, void *data) {

    #define rd ((ksnCorePacketData*)data)

    if(strcmp(peer_name, rd->from)) {
        ksnCommandSendCmdConnect( ((ksnetEvMgrClass*) ka->ke)->kc->kco,
                peer_name, rd->from, rd->addr, rd->port);
    }

    return 0;
    #undef rd
}

/**
 * Send connect command
 *
 * @param ka Pointer to ksnetArpClass
 * @param peer_name Peer name
 * @param arp_data Pointer to ARP data ksnet_arp_data
 * @param data Pointer to ksnCorePacketData
 */
static int send_cmd_connect_cb_b(ksnetArpClass *ka, char *peer_name,
                        ksnet_arp_data_ext *arp, void *data) {

    #define rd ((ksnCorePacketData*)data)

    if(strcmp(peer_name, rd->from)) {
        ksnCommandSendCmdConnectA( ((ksnetEvMgrClass*) ka->ke)->kc->kco, 
            rd->addr, rd->port, peer_name, arp->data.addr, arp->data.port);
    }

    return 0;
    #undef rd
}

/**
 * Process CMD_CONNECT_R command. A Peer want connect to r-host
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_connect_r_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_CONNECT_R (cmd = %u) command, from %s connect address: %s:%d\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Replay to address we got from peer
    ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_NONE, NULL_STR, 1);

    // Parse command data
    size_t i, ptr;
    ksnCorePacketData lrd;
    uint8_t *num_ip = rd->data; // Number of IPs

    // For UDP connection resend received IPs to child
    if(*num_ip) {

	for(int j=0; j < 1; j++) {
	    ptr = sizeof(uint8_t);
    	    lrd.port = *((uint32_t*)(rd->data + rd->data_len - sizeof(uint32_t)));
    	    lrd.from = rd->from;
    	    for(i = 0; i <= *num_ip; i++ ) {

        	if(!i) lrd.addr = (char*)localhost;
        	else {
            	    lrd.addr = rd->data + ptr; ptr += strlen(lrd.addr) + 1;
        	}

        	// Send local IP address and port to child
        	ksnetArpGetAll( ((ksnCoreClass*)kco->kc)->ka, send_cmd_connect_cb,
                    &lrd);
    	    }

    	    // Send peer address to child
    	    ksnetArpGetAll( ((ksnCoreClass*)kco->kc)->ka, send_cmd_connect_cb, rd);
            
            // Send child address to peer
            ksnetArpGetAll( ((ksnCoreClass*)kco->kc)->ka, send_cmd_connect_cb_b, rd);
        }
    }

    // For TCP proxy connection resend this host IPs to child
    else {

        rd->arp->data.mode = 2;
        lrd.port = rd->arp->data.port;
        lrd.from = rd->from;

        // Get this server IPs array
        ksnet_stringArr ips = getIPs(
                &((ksnetEvMgrClass*)(((ksnCoreClass*)kco->kc)->ke))->ksn_cfg );
        uint8_t ips_len = ksnet_stringArrLength(ips); // Number of IPs
        int i;
        for(i = 0; i <= ips_len; i++) {

            if(!i) lrd.addr = (char*)localhost;
            else if(ip_is_private(ips[i-1])) lrd.addr = ips[i-1];
            else continue;

            // Send local addresses for child
            ksnetArpGetAll( ((ksnCoreClass*)kco->kc)->ka, send_cmd_connect_cb,
                    &lrd);
        }
        ksnet_stringArrFree(&ips);

        // Send main peer address to child
        lrd.addr = rd->arp->data.addr;
        ksnetArpGetAll( ((ksnCoreClass*)kco->kc)->ka, send_cmd_connect_cb,
                &lrd);
    }

    return 1;
}

typedef struct cmd_connect_cque_cb_data {
    ksnetEvMgrClass* ke;
    char * addr;
    int port;
} cmd_connect_cque_cb_data;

static void cmd_connect_cque_cb(uint32_t id, int type, void *data) {
    if(data) {
        cmd_connect_cque_cb_data *cqd = data;
        
        char *peer_name = "";        
        struct sockaddr_in remaddr;         // remote address
        socklen_t addr_len = sizeof(remaddr);// length of addresses
        make_addr(cqd->addr, cqd->port, (__SOCKADDR_ARG) &remaddr, &addr_len);        
        ksnet_arp_data *arp = ksnetArpFindByAddr(cqd->ke->kc->ka, (__CONST_SOCKADDR_ARG) &remaddr, &peer_name);
        #ifdef DEBUG_KSNET
        ksn_printf(cqd->ke, MODULE, DEBUG_VV, 
                "processing CMD_CONNECT cmd_connect_cque_cb, %s, %s:%d %s\n", 
                peer_name, cqd->addr, cqd->port, arp ? " - connected" : " - remove trudp channel");
        #endif
        if(!arp) trudpChannelDestroyAddr(cqd->ke->kc->ku, cqd->addr, cqd->port, 0);
        free(cqd->addr);
        free(data);
    }
}

/**
 * Process CMD_CONNECT command. A peer send his child to us
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_connect_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {
    
    #define kev ((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke)

    /**
     * KSNet CMD_PEER command data
     */
    typedef struct ksnCmdPeerData {

        char *name;     ///< Peer name
        char *addr;     ///< Peer IP address
        uint32_t port;  ///< Peer port

    } ksnCmdPeerData;

    ksnCmdPeerData pd;

    // Parse command data
    size_t ptr = 0;
    pd.name = rd->data; ptr += strlen(pd.name) + 1;
    pd.addr = rd->data + ptr; ptr += strlen(pd.addr) + 1;
    pd.port = *((uint32_t *)(rd->data + ptr));

    #ifdef DEBUG_KSNET
    ksn_printf(kev, MODULE, DEBUG_VV,
            "process CMD_CONNECT (cmd = %u) to %s (%s:%d), got from %s (%s:%d)\n", 
            rd->cmd, pd.name, pd.addr, pd.port, rd->from, rd->addr, rd->port);
    #endif

    // Check ARP
    if(ksnetArpGet(((ksnCoreClass*)kco->kc)->ka, pd.name) == NULL) {
        
        // Send CMD_NONE to remote peer to connect to it
        ksnCoreSendto(kco->kc, pd.addr, pd.port, CMD_NONE, NULL_STR, 1);
        
    }
    else {
        #ifdef DEBUG_KSNET
        ksn_printf(kev, MODULE, DEBUG_VV,
                "processing CMD_CONNECT from already existing peer %s (%s:%d) - ignore it\n",
                pd.name, pd.addr, pd.port);
        #endif
    }
    
    // Wait connection 2 sec and remove TRUDP channel in callback if not connected
    cmd_connect_cque_cb_data *cqd = malloc(sizeof(cmd_connect_cque_cb_data));
    cqd->ke = kev;
    cqd->addr = strdup(pd.addr);
    cqd->port = pd.port;
    ksnCQueAdd(kev->kq, cmd_connect_cque_cb, 2.000, cqd);

    return 1;
    
    #undef kev
}

/**
 * Process CMD_DISCONNECTED command. A peer send disconnect command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
int cmd_disconnected_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_DISCONNECTED (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    #define kev ((ksnetEvMgrClass*) ((ksnCoreClass *) kco->kc)->ke)

    // Name of peer to disconnect
    char *peer_name = rd->from;
    if(rd->data != NULL && ((char*)rd->data)[0]) peer_name = rd->data;

    // Check r-host disconnected
    int is_rhost = kev->ksn_cfg.r_host_name[0] &&
                   !strcmp(kev->ksn_cfg.r_host_name,rd->from);
    if(is_rhost) kev->ksn_cfg.r_host_name[0] = '\0';

    // Try to reconnect, send CMD_RECONNECT command
    if(!is_rhost)
        ((ksnReconnectClass*)kco->kr)->send(((ksnReconnectClass*)kco->kr),
                peer_name);

    // Send event callback
    if(kev->event_cb != NULL)
        kev->event_cb(kev, EV_K_DISCONNECTED, (void*)rd, sizeof(*rd), NULL);

    // Send event to subscribers
    teoSScrSend(kev->kc->kco->ksscr, EV_K_DISCONNECTED, rd->from, rd->from_len, 0);

    // Remove from ARP Table
    ksnetArpRemove(((ksnCoreClass*)kco->kc)->ka, peer_name);

    return 1;

    #undef kev
}

/**
 * Process CMD_SPLIT command. A peer send disconnect command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_split_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke),
            MODULE, DEBUG_VV,
            "process CMD_SPLIT (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    #define kev ((ksnetEvMgrClass*) ((ksnCoreClass *) kco->kc)->ke)

    int processed = 0;

    ksnCorePacketData *rds = ksnSplitCombine(kco->ks, rd);
    if(rds != NULL) {
        processed = ksnCommandCheck(kco, rds);
        if(!processed) {

            // Send event callback
            if(kev->event_cb != NULL)
               kev->event_cb(kev, EV_K_RECEIVED, (void*)rds, sizeof(rds), NULL);

            processed = 2;
        }
        ksnSplitFreeRds(kco->ks, rds);
    }
    else processed = 1;

    return processed;

    #undef kev
}

