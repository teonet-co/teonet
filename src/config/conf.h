/**
 * File:   conf.h
 * Author: Kirill Scherba
 *
 * Created on April 11, 2015, 6:10 AM
 *
 * Configuration parameters module
 *
 */

#ifndef CONF_H
#define	CONF_H

#define KSN_BUFFER_DB_SIZE 2048        ///< Size of buffer for UDP packets
#define KSN_BUFFER_SIZE 1024           ///< Size of buffer for string variables
#define KSN_BUFFER_SM_SIZE 256         ///< Size of small buffer for string
#define KSN_BUFFER_64_SIZE 64          ///< Size of small buffer for string
#define KSN_BUFFER_32_SIZE 32          ///< Size of small buffer for string

#define KSN_MAX_HOST_NAME 31
#define NUMBER_TRY_PORTS 1000

#ifndef RELEASE_KSNET
#define DEBUG_KSNET  1
#endif

#define KSNET_CRYPT  0

typedef struct ksnet_cfg {

    // Flags
    int show_connect_f,    ///< Show connection message
        show_debug_f,     ///< Show debug messages
        show_debug_vv_f, ///< Show debug vv messages
        show_peers_f,   ///< Show peers at start up
        hot_keys_f,    ///< Show hotkeys when press h
        crypt_f,      ///< Encrypt/Decrypt packets
        vpn_connect_f;  ///< VPN start flag

    // Network
    char network[KSN_BUFFER_SM_SIZE/2];     ///< Network

    // Application name
    char app_prompt[KSN_BUFFER_SM_SIZE/2];      ///< Application prompt
    char app_name[KSN_BUFFER_SM_SIZE/2];        ///< Application name
    char app_description[KSN_BUFFER_SM_SIZE/2]; ///< Application description

    // Host name & port
    long port;                              ///< This host port number
    int  port_inc_f;                        ///< Increment host port if busy
    char host_name[KSN_MAX_HOST_NAME];      ///< This host name

    // R-Host
    char r_host_addr[KSN_BUFFER_SM_SIZE/2]; ///< Remote host internet address
    long r_port;                            ///< Remote host port

    // VPN
    char vpn_dev_name[KSN_MAX_HOST_NAME];
    char vpn_dev_hwaddr[KSN_MAX_HOST_NAME];
    char vpn_ip[KSN_MAX_HOST_NAME];
    long vpn_ip_net;

    // Helpers
    int pp;
    char pn[KSN_BUFFER_SM_SIZE];
    char r_host_name[KSN_MAX_HOST_NAME];    ///< Remote host name (if connected)

} ksnet_cfg;

#ifdef	__cplusplus
extern "C" {
#endif

void ksnet_addHWAddrConfig(ksnet_cfg *conf, char *hwaddr);
void read_config(ksnet_cfg *conf, int port_param);
void ksnet_configInit(ksnet_cfg *ksn_cfg);

#ifdef	__cplusplus
}
#endif

#endif	/* CONF_H */
