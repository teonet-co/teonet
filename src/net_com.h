/**
 * File:   net_com.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 29, 2015, 7:57 PM
 *
 * KSNet Network command processing module
 */


#ifndef NET_COM_H
#define	NET_COM_H

//#include "net_recon.h"

enum ksnCMD {

    // Core level not TR-UDP mode: 0...63
    CMD_NONE = 0,           ///< #0 Common command
    CMD_1_RESERVED,         ///< #1 Reserver for future use
    CMD_2_RESERVED,         ///< #2 Reserver for future use
    CMD_3_RESERVED,         ///< #3 Reserver for future use
    CMD_CONNECT_R,          ///< #4 A Peer want connect to r-host
    CMD_CONNECT,            ///< #5 Inform peer about connected peer
    CMD_DISCONNECTED,       ///< #6 Inform peer about disconnected peer
    CMD_VPN,                ///< #7 VPN command
    CMD_RESET,              ///< #8 Reset command, data: byte or char 0 - soft reset; 1 - hard reset

    // Core level TR-UDP mode: 64...127
    CMD_64_RESERVED = 64,   ///< #64 Reserver for future use
    CMD_ECHO,               ///< #65 Echo test message: auto replay test message command
    CMD_ECHO_ANSWER,        ///< #66 Answer to auto replay message command
    CMD_TUN,                ///< #67 Tunnel command
    CMD_SPLIT,              ///< #68 Group of packets (Splited packets)
    CMD_STREAM,             ///< #69 Stream command
    CMD_L0,                 ///< #70 Command from L0 Client
    CMD_L0_TO,              ///< #71 Command to L0 Client
    CMD_PEERS,              ///< #72 Get peers, allow JSON in request
    CMD_PEERS_ANSWER,       ///< #73 Get peers answer
    CMD_RESEND,             ///< #74 Resend command
    CMD_RECONNECT,          ///< #75 Reconnect command
    CMD_RECONNECT_ANSWER,   ///< #76 Reconnect answer command
    CMD_AUTH,               ///< #77 Auth command
    CMD_AUTH_ANSWER,        ///< #78 Auth answer command
    CMD_L0_CLIENTS,         ///< #79 Request clients list
    CMD_L0_CLIENTS_ANSWER,  ///< #80 Clients list
    CMD_SUBSCRIBE,          ///< #81 Subscribe to event
    CMD_UNSUBSCRIBE,        ///< #82 UnSubscribe from event
    CMD_SUBSCRIBE_ANSWER,   ///< #83 Subscribe answer
    CMD_L0_CLIENTS_N,       ///< #84 Request clients number, allow JSON in request
    CMD_L0_CLIENTS_N_ANSWER,///< #85 Clients number
    CMD_GET_NUM_PEERS,      ///< #86 Request number of peers, allow JSON in request
    CMD_GET_NUM_PEERS_ANSWER,///< #87 Number of peers answer
    CMD_L0_STAT,            ///< #88 Get LO server statistic request, allow JSON in request
    CMD_L0_STAT_ANSWER,     ///< #89 LO server statistic 
    CMD_HOST_INFO,          ///< #90 Request host info, allow JSON in request
    CMD_HOST_INFO_ANSWER,   ///< #91 Host info amswer
    CMD_L0_INFO,            ///< #92 L0 server info request
    CMD_L0_INFO_ANSWER,     ///< #93 L0 server info answer
    CMD_TRUDP_INFO,         ///< #94 TR-UDP info request
    CMD_TRUDP_INFO_ANSWER,  ///< #95 TR-UDP info answer
    
    CMD_L0_AUTH,            ///< #96 L0 server auth request answer command

    // Application level TR-UDP mode: 128...191
    CMD_128_RESERVED = 128, ///< #128 Reserver for future use
    CMD_USER,               ///< #129 User command

    // Application level not TR-UDP mode: 192...254
    CMD_192_RESERVED = 192, ///< #192 Reserver for future use
    CMD_USER_NR,            ///< #193 User command
    CMD_LAST = 255          ///< #255 Last command Reserved for future use
};

#define CMD_TRUDP_CHECK(CMD) (CMD >= CMD_64_RESERVED && CMD < CMD_192_RESERVED)

/**
 * KSNet command class data
 */
typedef struct ksnCommandClass {

    void *kc; ///< Pointer to KSNet core class object
    void *ks; ///< Pointer to KSNet split class
    void *kr; ///< Pointer to KSNet reconnect class
    void *ksscr; ///< Pointer to teoSScrClass

} ksnCommandClass;

/**
 * KSNet core received data structure
 */
typedef struct ksnCorePacketData {

    char *addr;             ///< Remote peer IP address
    int port;               ///< Remote peer port
    int mtu;                ///< Remote mtu
    char *from;             ///< Remote peer name
    uint8_t from_len;       ///< Remote peer name length

    uint8_t cmd;            ///< Command ID

    void *data;             ///< Received data
    size_t data_len;        ///< Received data length

    void *raw_data;         ///< Received packet data
    size_t raw_data_len;    ///< Received packet length

    ksnet_arp_data *arp;    ///< Pointer to ARP Table data

    int l0_f;               ///< L0 command flag (from set to l0 client name)

} ksnCorePacketData;


#ifdef	__cplusplus
extern "C" {
#endif

ksnCommandClass *ksnCommandInit(void *kc);
void ksnCommandDestroy(ksnCommandClass *kco);
int ksnCommandCheck(ksnCommandClass *kco, ksnCorePacketData *rd);
int ksnCommandSendCmdEcho(ksnCommandClass *kco, char *to, void *data, 
  size_t data_len);
void *ksnCommandEchoBuffer(ksnCommandClass *kco, void *data, size_t data_len, 
        size_t *data_t_len);
int ksnCommandSendCmdConnect(ksnCommandClass *kco, char *to, char *name, 
  char *addr, uint32_t port);

int cmd_disconnected_cb(ksnCommandClass *kco, ksnCorePacketData *rd);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_COM_H */
