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

//#ifndef RELEASE_KSNET
#define DEBUG_KSNET  1
//#endif

#define KSNET_EVENT_MGR_TIMER 0.25  ///< Main event manager timer interval
#define KSNET_PORT_DEFAULT "9000" ///< Main network port
#define KSNET_CRYPT  1 ///< Crypt packages

// Modules enable
#define M_ENAMBE_CQUE 1
#define M_ENAMBE_STREAM 1
#define M_ENAMBE_PBLKF 1
#define M_ENAMBE_VPN 1
#define M_ENAMBE_TCP 1
#define M_ENAMBE_L0s 1
#define M_ENAMBE_TCP_P 1
#define M_ENAMBE_TUN 1
#define M_ENAMBE_TERM 1
#define M_ENAMBE_ASYNC 1
#define M_ENAMBE_METRIC 1
#define M_ENAMBE_LOGGING_SERVER 1
#define M_ENAMBE_LOGGING_CLIENT 1
#define M_ENAMBE_LOG_READER 1

// TRUE & FALSE define
#define TRUE  1
#define FALSE 0

typedef struct ksnet_cfg {

    void *ke; ///< Poiner to ksnetEventManager

    // Flags
    int show_connect_f,         ///< Show connection message
        show_debug_f,           ///< Show debug messages
        show_debug_vv_f,        ///< Show debug vv messages
        show_debug_vvv_f,       ///< Show debug vvv messages
        show_peers_f,           ///< Show peers at start up
        hot_keys_f,             ///< Show hotkeys when press h
        crypt_f,                ///< Encrypt/Decrypt packets
        vpn_connect_f,          ///< Start VPN flag
        show_tr_udp_f,          ///< Show TR-UDP statistic at start up 
        send_ack_event_f,       ///< Send TR-UDP ACK event (EV_K_RECEIVED_ACK) to the teonet event loop
        sig_segv_f,             ///< SIGSEGV processing
        block_cli_input_f,      ///< Block teonet CLI input (for using in GUI application)
        logging_f,              ///< Start logging server
        log_disable_f,          ///< Disable send log to logging server 
        send_all_logs_f,        ///< Send all logs to logging server (by default only ###)
        color_output_disable_f, ///< Disable color output flag
        extended_l0_log_f,      ///< Extended L0 log output flag
        no_multi_thread_f;      ///< Don't try multi thread mode in async calls 
    
    // Daemon mode flags
    int dflag,  ///< Start application in Daemon mode
        kflag;  ///< Kill application in Daemon mode

    // Network
    char network[KSN_BUFFER_SM_SIZE/2];     ///< Network
    char net_key[KSN_BUFFER_SM_SIZE/2];     ///< Network key

    // Application name
    char app_prompt[KSN_BUFFER_SM_SIZE/2];      ///< Application prompt
    char app_name[KSN_BUFFER_SM_SIZE/2];        ///< Application name
    char app_description[KSN_BUFFER_SM_SIZE/2]; ///< Application description
    
    // Application parameters
    int app_argc; ///< Number of requered application parameters    
    char **app_argv; ///< Array of application parameters

    // Host name & port
    long port;                              ///< This host port number
    int  port_inc_f;                        ///< Increment host port if busy
    char host_name[KSN_MAX_HOST_NAME];      ///< This host name
    
    // TCP Proxy
    int  tcp_allow_f;       ///< Allow TCP Proxy connections to this host
    long tcp_port;          ///< TCP Proxy port number
    
    // L0 Server
    int  l0_allow_f;                             ///< Allow L0 Server and l0 client connections to this host
    char l0_tcp_ip_remote[KSN_BUFFER_SM_SIZE/2]; ///< L0 Server remote IP address (send clients to connect to server)
    long l0_tcp_port;                            ///< L0 Server TCP port number
    
    // Display log filter
    char filter[KSN_BUFFER_SM_SIZE/2];      ///<  Display log filter

    // R-Host
    char r_host_addr[KSN_BUFFER_SM_SIZE/2]; ///< Remote host internet address
    long r_port;                            ///< Remote host port
    long r_tcp_port;                        ///< Remote host tcp port
    int r_tcp_f;                            ///< Connect to TCP Proxy R-Host  

    // VPN
    char vpn_dev_name[KSN_MAX_HOST_NAME];   ///< VPN Interface device name
    char vpn_dev_hwaddr[KSN_MAX_HOST_NAME]; ///< VPN Interface MAC address
    char vpn_ip[KSN_BUFFER_SM_SIZE/2];      ///< VPN Interface IP
    long vpn_ip_net;                        ///< VPN Interface network mask
    long vpn_mtu;                           ///< VPN Interface MTU
    
    // Terminal
//    char t_username[KSN_BUFFER_SM_SIZE/2]; ///< User name to login to terminal
//    char t_password[KSN_BUFFER_SM_SIZE/2]; ///< Password to login to terminal
    
    // Syslog options
    long log_priority;                       ///< Syslog priority 

    // StatsD address
    char statsd_ip[KSN_BUFFER_SM_SIZE/2];
    long statsd_port;
    
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
void ksnet_configInit(ksnet_cfg *ksn_cfg, void *ke);

#ifdef	__cplusplus
}
#endif

#endif	/* CONF_H */
