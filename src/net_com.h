/**
 * File:   net_com.c
 * Author: Kirill Scherba
 *
 * Created on April 29, 2015, 7:57 PM
 *
 * KSNet Network command processing module
 */


#ifndef NET_COM_H
#define	NET_COM_H

enum ksnCMD {

    CMD_NONE = 0,       ///< Common command
    CMD_CONNECT_R,      ///< A Peer want connect to r-host
    CMD_CONNECT,        ///< Inform peer about connected peer
    CMD_DISCONNECTED,   ///< Inform peer about disconnected peer
    CMD_ECHO,           ///< Echo test message: auto replay test message command
    CMD_ECHO_ANSWER,    ///< Answer to auto replay message command
    CMD_VPN,            ///< VPN command
    CMD_TUN,            ///< Tunnel command

    CMD_USER = 128      ///< User command

};

/**
 * KSNet command class data
 */
typedef struct ksnCommandClass {

    void *kc; ///< Pointer to KSNet core class object

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
