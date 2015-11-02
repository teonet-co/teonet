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
#include "utils/rlutil.h"

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
int cmd_stream_cb(ksnStreamClass *ks, ksnCorePacketData *rd);
int cmd_l0_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd);
int cmd_l0to_cb(ksnetEvMgrClass *ke, ksnCorePacketData *rd);
int cmd_peers_cb(ksnCommandClass *kco, ksnCorePacketData *rd);
int cmd_resend_cb(ksnCommandClass *kco, ksnCorePacketData *rd);

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

    #define kev ((ksnetEvMgrClass*) ((ksnCoreClass *) kco->kc)->ke)

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
                kev->kvpn,
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
                    if(kev->event_cb != NULL)
                       kev->event_cb(kev, EV_K_RECEIVED, (void*)rds, sizeof(rds), 
                                NULL);

                    processed = 1;
                }
                ksnSplitFreeRds(kco->ks, rds);
            }
            else processed = 1;
        }
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
        case CMD_L0TO:
            processed = cmd_l0to_cb(kev, rd);
            break;
        #endif

        case CMD_PEERS:
            processed = cmd_peers_cb(kco, rd);
            break;
            
        case CMD_RESEND:
            processed = cmd_resend_cb(kco, rd);
            break;

        default:
            break;
    }

    return processed;
    
    #undef kev
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
 * @param kco
 * @param rd
 * @return 
 */
int cmd_echo_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {
         
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
 * Process CMD_PEERS
 *
 * @param kco
 * @param rd
 * @return 
 */
int cmd_peers_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {
        
    // Get peers data
    ksnet_arp_data_ar *peers_data = ksnetArpShowData(((ksnCoreClass*)kco->kc)->ka);
    size_t peers_data_length = ksnetArpShowDataLength(peers_data);
    
    // Send PEERS_ANSWER to L0 user
    if(rd->l0_f)
        ksnLNullSendToL0(((ksnetEvMgrClass*)((ksnCoreClass*)kco->kc)->ke), 
                rd->addr, rd->port, rd->from, rd->from_len, CMD_PEERS_ANSWER, 
                peers_data, peers_data_length);
    
    // Send PEERS_ANSWER to peer
    else
        ksnCoreSendto(kco->kc, rd->addr, rd->port, CMD_PEERS_ANSWER,
                peers_data, peers_data_length);
    
    return 1; // Command processed
}

/**
 * Process CMD_RESEND
 * 
 * The RESEND command sent by sender to peer if sender does not connected to 
 * peer. This function resend command to peer and send CONNECT command to sender 
 * and peer to connect it direct.
 *
 * @param kco
 * @param rd
 * @return 
 */
int cmd_resend_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {
    
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
    if((arp = ksnetArpGet(((ksnCoreClass*)kco->kc)->ka, to)) != NULL) {
        
        // Send connect command request to peer
        ksnCommandSendCmdConnect(kco, to, rd->from, rd->addr, rd->port);
        
        // Send connect command request to sender
        ksnCommandSendCmdConnect(kco, rd->from, to, arp->addr, arp->port);
    }
    
    return 1; // Command processed
}

/**
 * Process CMD_ECHO_ANSWER
 *
 * @param kco
 * @param rd
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
//        #ifdef DEBUG_KSNET
        ksnet_printf(& ke->ksn_cfg, MESSAGE,
            "%d bytes from %s: cmd=cmd_echo ttl=57 time=%.3f ms\n",
            rd->data_len, // command data length
            rd->from,     // from
            triptime
        );
//        #endif
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
            "%sNet command:%s "
            "received Echo answer command => from '%s': %d byte data: %s, %.3f ms\n",
            ANSI_LIGHTBLUE, ANSI_NONE,
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
    
    // For UDP connection resend received IPs to child
    if(*num_ip) {
        
        lrd.port = *((uint32_t*)(rd->data + rd->data_len - sizeof(uint32_t))); // Port number
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

    }
    
    // For TCP proxy connection resend this host IPs to child
    else {
        
        rd->arp->mode = 2;
        lrd.port = rd->arp->port;
        lrd.from = rd->from;
        
        // Get this server IPs array
        ksnet_stringArr ips = getIPs(); 
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
        lrd.addr = rd->arp->addr;
        ksnetArpGetAll( ((ksnCoreClass*)kco->kc)->ka, send_cmd_connect_cb, 
                &lrd);
    }

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
//    if(ksnetArpGet(((ksnCoreClass*)kco->kc)->ka, pd.name) == NULL) {

    if(ksnetArpGet(((ksnCoreClass*)kco->kc)->ka, pd.name) != NULL) {
        ksnTRUDPresetAddr(((ksnCoreClass*)kco->kc)->ku, pd.addr, pd.port, 0);
    }
    ksnCoreSendto(kco->kc, pd.addr, pd.port, CMD_NONE, NULL_STR, 1);
//    }

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
    
    // Name of peer to disconnect
    char *peer_name = rd->from;
    if(rd->data != NULL && ((char*)rd->data)[0]) peer_name = rd->data;
    
    // Check r-host disconnected
    int is_rhost = kev->ksn_cfg.r_host_name[0] && 
                   !strcmp(kev->ksn_cfg.r_host_name,rd->from);
    if(is_rhost) kev->ksn_cfg.r_host_name[0] = '\0';

    // Remove from ARP Table
    ksnetArpRemove(((ksnCoreClass*)kco->kc)->ka, peer_name);
    
    // Try to reconnect and resend Echo command through r-host:
    //
    // Send Echo command to disconnected peer. This command will be resend to 
    // r-host. R-Host will try to resend this command to peer. If peer has 
    // connected to r-host, r-host will send Connect commands to peer and this 
    // host
    if(!is_rhost) 
        ksnCommandSendCmdEcho(kco, peer_name, (void*) TRIPTIME, TRIPTIME_LEN);

    // Send event callback
    if(kev->event_cb != NULL)
        kev->event_cb(kev, EV_K_DISCONNECTED, (void*)rd, sizeof(*rd), NULL);

    return 1;

    #undef kev
}
