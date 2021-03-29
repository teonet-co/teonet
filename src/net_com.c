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
#include "trudp_stat.h"


// Local functions
static int cmd_echo_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_echo_unr_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_echo_answer_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_connect_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
static int cmd_connect_r_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_stream_cb(ksnStreamClass *ks, ksnCorePacketData *rd);
int cmd_l0_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd);
int cmd_l0_to_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd);
int cmd_l0_broadcast_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd);
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
static int cmd_get_public_ip_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
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
 
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    int processed = 0;

    switch(rd->cmd) {

        case CMD_NONE:
            ksn_printf(ke, MODULE, DEBUG_VV, "recieve CMD_NONE = %u from %s (%s:%d).\n",
                CMD_NONE, rd->from, rd->addr, rd->port);
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

        case CMD_ECHO_UNRELIABLE:
            processed = cmd_echo_unr_cb(kco, rd);
            break;

//        case CMD_ECHO_UNR_ANSWER:
//            processed = cmd_echo_unr_answer_cb(kco, rd);
//            break;

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
            processed = cmd_vpn_cb(ke->kvpn, rd->from, rd->data, rd->data_len);
            break;
        #endif

        case CMD_SPLIT:
            processed = cmd_split_cb(kco, rd);
            break;

        #ifdef M_ENAMBE_TUN
        case CMD_TUN:
            processed = cmd_tun_cb(ke->ktun, rd);
            break;
        #endif

        #ifdef M_ENAMBE_STREAM
        case CMD_STREAM:
            processed = cmd_stream_cb(ke->ks, rd);
            break;
        #endif

        #ifdef M_ENAMBE_L0s
        case CMD_L0:
            processed = cmd_l0_cb(ke, rd);
            break;
        #endif

        #ifdef M_ENAMBE_L0s
        case CMD_L0_TO:
            if(ke->kl) processed = cmd_l0_to_cb(ke, rd);
            break;
        #endif

        #ifdef M_ENAMBE_L0s
        case CMD_L0_CLIENT_BROADCAST:
            if(ke->kl) processed = cmd_l0_broadcast_cb(ke, rd);
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

        case CMD_GET_PUBLIC_IP:
            processed = cmd_get_public_ip_cb(kco, rd);
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
void *ksnCommandEchoBuffer(ksnCommandClass *kco, void *data, size_t data_len, size_t *data_t_len) {

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    double ct = ksnetEvMgrGetTime(ke);
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
int ksnCommandSendCmdEcho(ksnCommandClass *kco, char *to, void *data, size_t data_len) {

    size_t data_t_len;
    void *data_t = ksnCommandEchoBuffer(kco, data, data_len, &data_t_len);

    ksnet_arp_data * arp = ksnCoreSendCmdto(kco->kc, to, CMD_ECHO, data_t, data_t_len);

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    if(arp != NULL) {
        arp->last_triptime_send = ksnetEvMgrGetTime(ke);
    }

    free(data_t);
    return arp != NULL;
}

void fillConnectData(char *data, size_t *ptr, char *name, char *addr, uint32_t port) {
    strncpy(data, name, KSN_BUFFER_DB_SIZE); *ptr = strlen(name) + 1;
    strncpy(data + *ptr, addr, KSN_BUFFER_DB_SIZE - *ptr); *ptr += strlen(addr) + 1;
    *((uint32_t *)(data + *ptr)) = port; *ptr += sizeof(uint32_t);
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

    fillConnectData(data, &ptr, name, addr, port);

    // TODO: duplicate code
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "send CMD_CONNECT = %u to peer by name %s. (Connect to peer: %s, addr: %s:%d)\n",
        CMD_CONNECT, to, name, addr, port);
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
    fillConnectData(data, &ptr, name, addr, port);
    // TODO: duplicate code
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "send CMD_CONNECT = %u to peer by address %s:%d. (Connect to peer: %s, addr: %s:%d)\n",
        CMD_CONNECT, to_addr, to_port, name, addr, port);
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
    } else {// Soft reset
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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_ECHO (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Send ECHO to L0 user
    if(rd->l0_f) {
        ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_ECHO_ANSWER,
                rd->data, rd->data_len);
    } else {// Send echo answer command to peer
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_ECHO_ANSWER, rd->data, rd->data_len);
    }

    return 1; // Command processed
}

static int cmd_echo_unr_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_ECHO_UNR (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Send ECHO to L0 user
    ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_ECHO_UNRELIABLE_ANSWER,
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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    ksnetArpClass *arp_class = ARP_TABLE_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_PEERS (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get peers data
    ksnet_arp_data_ext_ar *peers_data = teoArpGetExtendedArpTable(arp_class);
    size_t peers_data_length = ARP_TABLE_DATA_LENGTH(peers_data);

    // Get type of request: 0 - binary; 1 - JSON
    const int data_type = rd->data_len && !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

    // Convert data to JSON format
    if(data_type == 1) {
        size_t peers_json_len;
        char *peers_json = teoArpGetExtendedArpTable_json(peers_data, &peers_json_len);
        free(peers_data);
        peers_data = (ksnet_arp_data_ext_ar *)peers_json;
        peers_data_length = peers_json_len;
    }

    // Send PEERS_ANSWER to L0 user
    if(rd->l0_f) {
        ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_PEERS_ANSWER, peers_data,
                peers_data_length);
    } else {// Send PEERS_ANSWER to peer
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_PEERS_ANSWER, peers_data, peers_data_length);
    }

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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    ksnetArpClass *arp_class = ARP_TABLE_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_GET_NUM_PEERS (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    void *peers_data;
    size_t peers_data_length = 0;

    // Get type of request: 0 - binary; 1 - JSON
    const int data_type = rd->data_len && !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

    // Get peers number data
    uint32_t peers_number = ksnetArpSize(arp_class); // Get peers number

    // JSON data type
    if(data_type == 1) {
        char *json_str = ksnet_formatMessage("{ \"numPeers\": %d }", peers_number);

        peers_data = json_str;
        peers_data_length = strlen(json_str) + 1;
    } else {// BINARY data type
        peers_data_length = sizeof(peers_number);
        peers_data = memdup(&peers_number, peers_data_length);
    }

    // Send PEERS_ANSWER to L0 user
    if(rd->l0_f) {
        ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_GET_NUM_PEERS_ANSWER,
                peers_data, peers_data_length);
    } else {// Send PEERS_ANSWER to peer
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_GET_NUM_PEERS_ANSWER, peers_data, peers_data_length);
    }

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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_L0_CLIENTS (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get l0 clients data
    teonet_client_data_ar *client_data = ksnLNullClientsList(ke->kl);

    if(client_data != NULL) {

        size_t client_data_length = ksnLNullClientsListLength(client_data);

        // Send CMD_L0_CLIENTS_ANSWER to L0 user
        if(rd->l0_f) {
            ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_L0_CLIENTS_ANSWER,
                    client_data, client_data_length);
        } else {// Send PEERS_ANSWER to peer
            ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_L0_CLIENTS_ANSWER, client_data,
                    client_data_length);
        }
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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_L0_CLIENTS_N (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get l0 clients data
    teonet_client_data_ar *client_data = ksnLNullClientsList(ke->kl);

    uint32_t clients_number = client_data == NULL ? 0 : client_data->length;

    char *json_str = NULL;
    void *data_out = &clients_number;
    size_t data_out_len = sizeof(clients_number);
    if(rd->data_len && !strncmp(rd->data, JSON, rd->data_len)) {

        json_str = ksnet_sformatMessage(json_str, "{ \"numClients\": %d }", clients_number);
        data_out = json_str;
        data_out_len = strlen(json_str) + 1;
    }

    // Send CMD_L0_CLIENTS_ANSWER to L0 user
    if(rd->l0_f) {
        ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_L0_CLIENTS_N_ANSWER,
                data_out, data_out_len);
    } else { // Send PEERS_ANSWER to peer
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_L0_CLIENTS_N_ANSWER, data_out, data_out_len);
    }

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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_L0_INFO (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    l0_info_data *info_d = NULL;
    size_t info_d_len = 0;

    // Get L0 info
    if(ke->kl != NULL) {
        if(ke->teo_cfg.l0_allow_f) {
            if(ke->teo_cfg.l0_tcp_ip_remote[0]) {

                size_t l0_tcp_ip_remote_len =  strlen(ke->teo_cfg.l0_tcp_ip_remote) + 1;
                info_d_len = sizeof(l0_info_data) + l0_tcp_ip_remote_len;
                info_d = malloc(info_d_len);

                memcpy(info_d->l0_tcp_ip_remote, ke->teo_cfg.l0_tcp_ip_remote, l0_tcp_ip_remote_len);
                info_d->l0_tcp_port = ke->teo_cfg.l0_tcp_port;
            }
        }
    }

    // Send CMD_L0_INFO_ANSWER to L0 user
    if(rd->l0_f) {
        ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_L0_INFO_ANSWER,
                info_d, info_d_len);
    } else { // Send L0_STAT_ANSWER to peer
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_L0_INFO_ANSWER, info_d, info_d_len);
    }

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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_L0_STAT (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get l0 clients statistic
    ksnLNullSStat *stat_data = ksnLNullStat(ke->kl);

    if(stat_data != NULL) {

        // Get type of request: 0 - binary; 1 - JSON
        const int data_type = rd->data_len && !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

        size_t l0_stat_data_len = sizeof(ksnLNullSStat);
        void *l0_stat_data = stat_data;
        char *json_str = NULL;

        // JSON data type
        if(data_type == 1) {
            json_str = ksnet_formatMessage("{ \"visits\": %d }", stat_data->visits);

            l0_stat_data = json_str;
            l0_stat_data_len = strlen(json_str) + 1;
        }

        // Send L0_STAT_ANSWER to L0 user
        if(rd->l0_f) {
            ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_L0_STAT_ANSWER,
                    l0_stat_data, l0_stat_data_len);
        } else {// Send L0_STAT_ANSWER to peer
            ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_L0_STAT_ANSWER, l0_stat_data, l0_stat_data_len);
        }
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
    
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    ksnetArpClass *arp_class = ARP_TABLE_OBJECT(kco);

    const int not_json = rd->data_len && ((char*)rd->data)[0] != '{' && ((char*)rd->data)[rd->data_len-1] != '}';
    
    int retval = 0;
    if (not_json) {
        ksnet_arp_data_ext *arp_cque =  ksnetArpGet(arp_class, rd->from);
        ksnCQueExec(ke->kq, arp_cque->cque_id_peer_type);
    
        if(!rd->arp->type) {
            
            host_info_data *hid = (host_info_data *)rd->data;
            char *type_str = teoGetFullAppTypeFromHostInfo(hid);

            // Add type to arp-table
            rd->arp->type = type_str;

            // Metrics
            char *met = ksnet_formatMessage("CON.%s", rd->from);
            teoMetricGauge(ke->tm, met, 1);
            free(met);

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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_HOST_INFO = %u command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get host info
    size_t hid_len;
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
            size_t ptr = strlen(hid->string_ar) + 1;
            char *type_str = strdup(null_str);
            for(int i = 1; i < hid->string_ar_num; i++) {

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
            ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_HOST_INFO_ANSWER,
                    data_out, data_out_len);
        // Send HOST_INFO_ANSWER to peer
        } else {
            ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_HOST_INFO_ANSWER, data_out, data_out_len);
            #ifdef DEBUG_KSNET
            ksn_printf(ke, MODULE, DEBUG_VV, "send CMD_HOST_INFO_ANSWER = %u command to (%s:%d)\n",
                CMD_HOST_INFO_ANSWER, rd->addr, rd->port);
            #endif
	    }

        // Free json string data
        if(json_str != NULL) free(json_str);
        free(hid);
    }

    return 1; // Command processed
}

/**
 * Process CMD_GET_PUBLIC_IP
 *
 * @param kco
 * @param rd
 * @return
 */
static int cmd_get_public_ip_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_GET_PUBLIC_IP (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    void *data_out = NULL;
    size_t data_out_len = 0;

    // Get type of request: 0 - binary; 1 - JSON
    const int data_type = rd->data_len &&
                !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

    // Get public ip

    // JSON data type
    if(data_type == 1) {
        data_out = ksnet_formatMessage(
            "{ "
                "\"public_v4\": \"%s\", "
                "\"public_v6\": \"%s\" "
            " }", 
            ke->teo_cfg.l0_public_ipv4, ke->teo_cfg.l0_public_ipv6);
        data_out_len = strlen(data_out) + 1;
    } else {
        size_t ipv4_len = strlen(ke->teo_cfg.l0_public_ipv4);
        size_t ipv6_len = strlen(ke->teo_cfg.l0_public_ipv6);
        data_out_len = ipv4_len + ipv6_len + 2;
        data_out = malloc(data_out_len*sizeof(char));
        memcpy(data_out, ke->teo_cfg.l0_public_ipv4, ipv4_len);
        ((char*)data_out)[ipv4_len] = '\0';
        memcpy((char*)data_out + ipv4_len + 1, ke->teo_cfg.l0_public_ipv6, ipv6_len);
        ((char*)data_out)[data_out_len - 1] = '\0';
    }

    // Send PEERS_ANSWER to L0 user
    if(rd->l0_f) {
        ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_GET_PUBLIC_IP_ANSWER,
                data_out, data_out_len);
    // Send HOST_INFO_ANSWER to peer
    } else {
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_GET_PUBLIC_IP_ANSWER, data_out, data_out_len);
    }

    // Free json string data
    free(data_out);

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
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_TRUDP_INFO (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Get type of request: 0 - binary; 1 - JSON
    const int data_type = rd->data_len && !strncmp(rd->data, JSON, rd->data_len)  ? 1 : 0;

    // Get TR-UDP info
    size_t data_out_len;
    void *data_out = trudpStatGet(ke->kc->ku, data_type, &data_out_len);

    if(rd->l0_f) {// Send TRUDP_INFO_ANSWER to L0 user
        ksnLNullSendToL0(ke, rd->addr, rd->port, rd->from, rd->from_len, CMD_TRUDP_INFO_ANSWER,
                data_out, data_out_len);
    } else {// Send TRUDP_INFO_ANSWER to peer
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_TRUDP_INFO_ANSWER, data_out, data_out_len);
    }

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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_RESEND (cmd = %u) command, from %s (%s:%d)\n",
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
    ksnetArpClass *arp_class = ARP_TABLE_OBJECT(kco);
    if((arp = (ksnet_arp_data *)ksnetArpGet(arp_class, to)) != NULL) {

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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_RECONNECT (cmd = %u) command, from %s (%s:%d)\n",
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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_RECONNECT_ANSWER (cmd = %u) command, from %s (%s:%d)\n",
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

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_ECHO_ANSWER (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    double time_got = ksnetEvMgrGetTime(ke);
    double time_send = *(double*)(rd->data + rd->data_len - sizeof(double));
    double triptime = (time_got - time_send) * 1000.0;

    // Set trip time and last tip time got
    rd->arp->data.last_triptime_got = time_got;
    rd->arp->data.last_triptime = triptime;
    rd->arp->data.triptime = (rd->arp->data.triptime + triptime) / 2.0;

    // Ping answer
    if(!strcmp(rd->data, PING)) {

        // Show command message
        //#ifdef DEBUG_KSNET
        ksnet_printf(&ke->teo_cfg, DISPLAY_M, "%d bytes from %s: cmd=cmd_echo ttl=57 time=%.3f ms\n",
            (int)rd->data_len, rd->from, triptime);
        //#endif
    }

    // Trim time answer
    else if(!strcmp(rd->data, TRIPTIME)) {
        // ...
    }

    // Monitor answer
    else if(!strcmp(rd->data, MONITOR)) {
        ksnet_printf(&ke->teo_cfg, DISPLAY_M, "%d bytes from %s: cmd=cmd_echo ttl=57 time=%.3f ms\n",
            (int)rd->data_len, rd->from, triptime);

        ksnetArpClass *arp_class = ARP_TABLE_OBJECT(kco);
        // Set monitor time
        ksnet_arp_data_ext *arp = ksnetArpGet(arp_class, rd->from);
        arp->data.monitor_time = time_got - time_send;
        ksnetArpAdd(arp_class, rd->from, arp);
    } else {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV, "got echo answer from \"%s\", %d byte data: \"%.*s\", %.3f ms\n",
            rd->from, rd->data_len,
            rd->data_len == 8 ? 0 : rd->data_len,    // TODO: why 8 bytes?
            rd->data, triptime);
        #endif
    }

    return 0;
}

/**
 * Send connect command
 *
 * @param ka Pointer to ksnetArpClass
 * @param peer_name Peer name
 * @param arp_data Pointer to ARP data ksnet_arp_data
 * @param data Pointer to ksnCorePacketData
 */
int send_cmd_connect_cb(ksnetArpClass *ka, char *peer_name,
                        ksnet_arp_data_ext *arp, void *data) {

    ksnCorePacketData *rd = data;

    if(strcmp(peer_name, rd->from)) {
        ksnCommandSendCmdConnect( ((ksnetEvMgrClass*) ka->ke)->kc->kco,
                peer_name, rd->from, rd->addr, rd->port);
    }

    return 0;
}

/**
 * Send connect command
 *
 * @param ka Pointer to ksnetArpClass
 * @param peer_name Peer name
 * @param arp_data Pointer to ARP data ksnet_arp_data
 * @param data Pointer to ksnCorePacketData
 */
int send_cmd_connect_cb_b(ksnetArpClass *ka, char *peer_name,
                        ksnet_arp_data_ext *arp, void *data) { 

    ksnCorePacketData *rd = data;

    if(strcmp(peer_name, rd->from)) {
        ksnCommandSendCmdConnectA( ((ksnetEvMgrClass*) ka->ke)->kc->kco, 
            rd->addr, rd->port, peer_name, arp->data.addr, arp->data.port);
    }

    return 0;
}

/**
 * Process CMD_CONNECT_R command. A Peer want connect to r-host
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_connect_r_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    ksnetArpClass *arp_class = ARP_TABLE_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV,
        "process CMD_CONNECT_R = %u command, from %s (%s:%d)\n",
        rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Set flag isRhost
    ke->is_rhost = true;

    // Replay to address we got from peer
    // ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_NONE, "\0", 2);
    ksnCoreSendCmdto(kco->kc, rd->from, CMD_NONE, "\0", 2);
    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "send CMD_NONE = %u to (%s:%d).\n",
            CMD_NONE, rd->addr, rd->port);
    #endif

    // Parse command data
    uint8_t *num_ip = rd->data; // Number of IPs

    // For UDP connection resend received IPs to child
    if(*num_ip) {
        // Send peer address to child
        ksnetArpGetAll(arp_class, send_cmd_connect_cb, rd);
        // Send child address to peer
        ksnetArpGetAll(arp_class, send_cmd_connect_cb_b, rd);
    } else {// For TCP proxy connection resend this host IPs to child
        rd->arp->data.mode = 2;
        ksnCorePacketData lrd;
        lrd.port = rd->arp->data.port;
        lrd.from = rd->from;

        // Get this server IPs array
        ksnet_stringArr ips = getIPs(&ke->teo_cfg);
        uint8_t ips_len = ksnet_stringArrLength(ips); // Number of IPs
        int i;
        for(i = 0; i <= ips_len; i++) {
            if(!i) {
                lrd.addr = (char*)localhost;
            } else if(ip_is_private(ips[i-1])) {
                lrd.addr = ips[i-1];
            } else {
                continue;
            }

            // Send local addresses for child
            ksnetArpGetAll(arp_class, send_cmd_connect_cb, &lrd);
        }

        ksnet_stringArrFree(&ips);

        // Send main peer address to child
        lrd.addr = rd->arp->data.addr;
        ksnetArpGetAll(arp_class, send_cmd_connect_cb, &lrd);
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
        struct sockaddr_storage remaddr;         // remote address
        socklen_t addrlen = sizeof(remaddr); // length of addresses
        make_addr(cqd->addr, cqd->port, (__SOCKADDR_ARG) &remaddr, &addrlen);
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
    
    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    ksnetArpClass *arp_class = ARP_TABLE_OBJECT(kco);

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
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_CONNECT = %u from %s (%s:%d). (Connect to %s (%s:%d))\n", 
            rd->cmd, rd->from, rd->addr, rd->port, pd.name, pd.addr, pd.port);
    #endif

    // Check ARP
    if(ksnetArpGet(arp_class, pd.name) == NULL) {
        // Send CMD_NONE to remote peer to connect to it
        ksnCoreSendto(kco->kc, pd.addr, pd.port, CMD_NONE, NULL_STR, 1);
        ksn_printf(ke, MODULE, DEBUG_VV, "send CMD_NONE = %u to (%s:%d).\n", 
            CMD_NONE, pd.addr, pd.port);
    } else {
        //#ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG_VV, "warning! We ignore this command CMD_CONNECT = %u, because peer %s (%s:%d) already connected\n",
                CMD_CONNECT, pd.name, pd.addr, pd.port);
        //#endif
    }
    
    // Wait connection 2 sec and remove TRUDP channel in callback if not connected
    cmd_connect_cque_cb_data *cqd = malloc(sizeof(cmd_connect_cque_cb_data));
    cqd->ke = ke;
    cqd->addr = strdup(pd.addr);
    cqd->port = pd.port;
    ksnCQueAdd(ke->kq, cmd_connect_cque_cb, 2.000, cqd);


    return 1;
}

/**
 * Process CMD_DISCONNECTED command. A peer send disconnect command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
int cmd_disconnected_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    ksnetArpClass *arp_class = ARP_TABLE_OBJECT(kco);

    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_DISCONNECTED = %u command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    // Name of peer to disconnect
    char *peer_name = rd->from;
    if(rd->data != NULL && ((char*)rd->data)[0]) {
        peer_name = rd->data;
    }

    // Metrics
    char *met = ksnet_formatMessage("CON.%s", peer_name);
    teoMetricGauge(ke->tm, met, 0);
    free(met);

    // Check r-host disconnected
    int is_rhost = ke->teo_cfg.r_host_name[0] && !strcmp(ke->teo_cfg.r_host_name,rd->from);
    if(is_rhost) {
        ke->teo_cfg.r_host_name[0] = '\0';
    }

    // Try to reconnect, send CMD_RECONNECT command
    if(!is_rhost) {
        ((ksnReconnectClass*)kco->kr)->send(((ksnReconnectClass*)kco->kr), peer_name);
    }

    // Send event callback
    if(ke->event_cb != NULL)
        ke->event_cb(ke, EV_K_DISCONNECTED, (void*)rd, sizeof(*rd), NULL);

    // Send event to subscribers
    teoSScrSend(kco->ksscr, EV_K_DISCONNECTED, rd->from, rd->from_len, 0);

    // Remove from ARP Table
    ksnetArpRemove(arp_class, peer_name);

    return 1;
}

/**
 * Process CMD_SPLIT command. A peer send disconnect command
 *
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return True if command is processed
 */
static int cmd_split_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {

    ksnetEvMgrClass *ke = EVENT_MANAGER_OBJECT(kco);
    
    #ifdef DEBUG_KSNET
    ksn_printf(ke, MODULE, DEBUG_VV, "process CMD_SPLIT (cmd = %u) command, from %s (%s:%d)\n",
            rd->cmd, rd->from, rd->addr, rd->port);
    #endif

    int processed = 0;
    ksnCorePacketData *rds = ksnSplitCombine(kco->ks, rd);

    if(rds != NULL) {
        processed = ksnCommandCheck(kco, rds);
        if(!processed) {
            // Send event callback
            if(ke->event_cb != NULL) {
               ke->event_cb(ke, EV_K_RECEIVED, (void*)rds, sizeof(rds), NULL);
            }
            processed = 2;
        }
        ksnSplitFreeRds(kco->ks, rds);
    } else {
        processed = 1;
    }

    return processed;
}

