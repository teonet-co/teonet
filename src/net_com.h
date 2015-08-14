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

enum ksnCMD {

    // Core level not TR-UDP mode: 0...63
    CMD_NONE = 0,           ///< #0 Common command
    CMD_1_RESERVED,         ///< #1 Reserver for future use
    CMD_2_RESERVED,         ///< #2 Reserver for future use
    CMD_3_RESERVED,         ///< #3 Reserver for future use
    CMD_CONNECT_R,          ///< A Peer want connect to r-host
    CMD_CONNECT,            ///< Inform peer about connected peer
    CMD_DISCONNECTED,       ///< Inform peer about disconnected peer
    CMD_VPN,                ///< VPN command
    
    // Core level TR-UDP mode: 64...127
    CMD_64_RESERVED = 64,   ///< #64 Reserver for future use
    CMD_ECHO,               ///< Echo test message: auto replay test message command
    CMD_ECHO_ANSWER,        ///< Answer to auto replay message command
    CMD_TUN,                ///< Tunnel command
    CMD_SPLIT,              ///< Group of packets (Splited packets)
            
    // Application level TR-UDP mode: 128...191
    CMD_128_RESERVED = 128, ///< #128 Reserver for future use
    CMD_USER,               ///< User command
            
    // Application level not TR-UDP mode: 192...255
    CMD_192_RESERVED = 192, ///< #192 Reserver for future use
    CMD_USER_NR             ///< User command
};

#define CMD_TRUDP_CHECK(CMD) (CMD >= CMD_64_RESERVED && CMD < CMD_192_RESERVED)
//static const int not_RTUDP[] = { CMD_NONE, CMD_CONNECT_R, CMD_CONNECT, CMD_DISCONNECTED, CMD_VPN };
//static const size_t not_RTUDP_len = sizeof(not_RTUDP) / sizeof(int);

/**
 * KSNet command class data
 */
typedef struct ksnCommandClass {

    void *kc; ///< Pointer to KSNet core class object
    void *ks; ///< Pointer to KSNet split class

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

} ksnCorePacketData;


#ifdef	__cplusplus
extern "C" {
#endif

ksnCommandClass *ksnCommandInit(void *kc);
void ksnCommandDestroy(ksnCommandClass *kco);
int ksnCommandCheck(ksnCommandClass *kco, ksnCorePacketData *rd);
int ksnCommandSendCmdEcho(ksnCommandClass *kco, char *to, void *data, size_t data_len);
int ksnCommandSendCmdConnect(ksnCommandClass *kco, char *to, char *name, char *addr, uint32_t port);

int cmd_disconnected_cb(ksnCommandClass *kco, ksnCorePacketData *rd);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_COM_H */
