/**
 * \file   conf.c
 * \author Kirill Scherba
 *
 * Created on April 11, 2015, 6:10 AM
 *
 * Teonet configuration parameters module
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

#include <confuse.h>

#include "config/conf.h"
#include "config/config.h"
#include "utils/utils.h"

extern const char *localhost;

// Local function
void set_defaults(ksnet_cfg *ksn_cfg);

/**
 * Get configuration parameters
 */
void ksnet_configInit(ksnet_cfg *ksn_cfg, void *ke) {

    ksn_cfg->ke = ke;
    set_defaults(ksn_cfg);
    //! \todo: Set port param
    //int port_param = 0;
    //read_config(ksn_cfg, port_param);
}

/**
 * Set default configuration parameters
 */
void set_defaults(ksnet_cfg *ksn_cfg) {

    // Encrypt/Decrypt packets
    ksn_cfg->crypt_f = KSNET_CRYPT;

    ksn_cfg->network[0] = '\0';

    // Show info at console flags
    ksn_cfg->show_connect_f = 1;
    ksn_cfg->show_debug_f = 1;
    ksn_cfg->show_debug_vv_f = 0;
    ksn_cfg->show_peers_f = 0;

    // This host
    ksn_cfg->port = atoi(KSNET_PORT_DEFAULT);
    ksn_cfg->port_inc_f = 1;
    char *name = getRandomHostName();
    strncpy(ksn_cfg->host_name, name, KSN_MAX_HOST_NAME);
    free(name);
    
    // TCP Proxy
    ksn_cfg->tcp_allow_f = 0;
    ksn_cfg->tcp_port = ksn_cfg->port;
            
    // Remote host default
    ksn_cfg->r_port = atoi(KSNET_PORT_DEFAULT);
    ksn_cfg->r_host_name[0] = '\0';
    ksn_cfg->r_host_addr[0] = '\0';
    ksn_cfg->r_tcp_f = 0;
    ksn_cfg->r_tcp_port = atoi(KSNET_PORT_DEFAULT);

    // VPN
    ksn_cfg->vpn_dev_name[0] = '\0';
    //strncpy(ksn_cfg->vpn_dev_name, "teonet", KSN_MAX_HOST_NAME);
    ksn_cfg->vpn_dev_hwaddr[0] = '\0';
    ksn_cfg->vpn_ip[0] = '\0';
    ksn_cfg->vpn_ip_net = 24;
    ksn_cfg->vpn_connect_f = 0;
    ksn_cfg->vpn_mtu = 0;
    
    // Terminal
//    strncpy(ksn_cfg->t_username, "fred", KSN_BUFFER_SM_SIZE/2);
//    strncpy(ksn_cfg->t_password, "nerk", KSN_BUFFER_SM_SIZE/2);
}

/**
 * Read configuration parameters from file
 * 
 * @param conf
 * @param port_param
 */
void read_config(ksnet_cfg *conf, int port_param) {

    // Save values back to structure
    #define save_conf_back() \
        strncpy(conf->host_name, host_name, KSN_MAX_HOST_NAME); \
        strncpy(conf->r_host_addr, r_host_addr, KSN_BUFFER_SM_SIZE/2); \
        strncpy(conf->vpn_ip, vpn_ip, KSN_MAX_HOST_NAME); \
        strncpy(conf->vpn_dev_name, vpn_dev_name, KSN_MAX_HOST_NAME); \
        strncpy(conf->vpn_dev_hwaddr, vpn_dev_hwaddr, KSN_MAX_HOST_NAME)

    // Load string values
    char *host_name = strdup(conf->host_name);
    char *r_host_addr = strdup(conf->r_host_addr);
    char *vpn_ip = strdup(conf->vpn_ip);
    char *vpn_dev_name = strdup(conf->vpn_dev_name);
    char *vpn_dev_hwaddr = strdup(conf->vpn_dev_hwaddr);

    cfg_opt_t opts[] = {

        CFG_SIMPLE_STR("host_name", &host_name),
        CFG_SIMPLE_INT("port", &conf->port),
        CFG_SIMPLE_BOOL("port_inc_f", &conf->port_inc_f),
        
        CFG_SIMPLE_INT("tcp_port", &conf->tcp_port),
        CFG_SIMPLE_BOOL("tcp_allow_f", &conf->tcp_allow_f),

        CFG_SIMPLE_STR("r_host_addr", &r_host_addr),
        CFG_SIMPLE_INT("r_port", &conf->r_port),
        
        CFG_SIMPLE_BOOL("r_tcp_f", &conf->r_tcp_f),
        CFG_SIMPLE_INT("r_tcp_port", &conf->r_tcp_port),        

        #if KSNET_CRYPT
        CFG_SIMPLE_BOOL("crypt_f", &conf->crypt_f),
        #endif

        CFG_SIMPLE_BOOL("show_connect_f", &conf->show_connect_f),
        CFG_SIMPLE_BOOL("show_debug_f", &conf->show_debug_f),
        CFG_SIMPLE_BOOL("show_debug_vv_f", &conf->show_debug_vv_f),
        CFG_SIMPLE_BOOL("show_peers_f", &conf->show_peers_f),
        CFG_SIMPLE_BOOL("show_tr_udp_f", &conf->show_tr_udp_f),
        CFG_SIMPLE_BOOL("hot_keys_f", &conf->hot_keys_f),
        CFG_SIMPLE_BOOL("daemon_mode_f", &conf->dflag),

        #if M_ENAMBE_VPN
        CFG_SIMPLE_BOOL("vpn_connect_f", &conf->vpn_connect_f),
        CFG_SIMPLE_STR("vpn_ip", &vpn_ip),
        CFG_SIMPLE_INT("vpn_ip_net", &conf->vpn_ip_net),
        CFG_SIMPLE_INT("vpn_mtu", &conf->vpn_mtu),
        CFG_SIMPLE_STR("vpn_dev_name", &vpn_dev_name),
        CFG_SIMPLE_STR("vpn_dev_hwaddr", &vpn_dev_hwaddr),
        #endif

        CFG_END()
    };

    int i;
    cfg_t *cfg;
    cfg = cfg_init(opts, 0);
    char buf[KSN_BUFFER_SIZE];

    // Open and parse common configure file in system and then in data directory
    for(i = 0; i < 2; i++) {

        if(!i) strncpy(buf, ksnet_getSysConfigDir(), KSN_BUFFER_SIZE);
        else strncpy(buf, getDataPath(), KSN_BUFFER_SIZE);
        if(conf->network[0]) {
            strncat(buf, "/", KSN_BUFFER_SIZE - strlen(buf) - 1);
            strncat(buf, conf->network, KSN_BUFFER_SIZE - strlen(buf) - 1);
        }
        strncat(buf, "/teonet.conf", KSN_BUFFER_SIZE - strlen(buf) - 1);
        if(access(buf, F_OK) != -1 ) {
            cfg_parse(cfg, buf);
            save_conf_back();
        }

        // Print the parsed values to save configuration file
        {
            strncpy(buf, getDataPath(), KSN_BUFFER_SIZE);
            if(conf->network[0]) {
                strncat(buf, "/", KSN_BUFFER_SIZE - strlen(buf) - 1);
                strncat(buf, conf->network, KSN_BUFFER_SIZE - strlen(buf) - 1);
            }
            strncat(buf, "/teonet.conf.out", KSN_BUFFER_SIZE - strlen(buf) - 1);
            char *dir = strdup(buf);
            #if HAVE_MINGW
            mkdir(dirname(dir));
            #else
            mkdir(dirname(dir), 0755);
            #endif
            free(dir);
            FILE *fp = fopen(buf, "w");
            cfg_print(cfg, fp);
            fclose(fp);
        }
    }

    // Open and parse unique port file configuration
    if(port_param) {
        char *uconf = ksnet_formatMessage("/teonet-%d.conf", port_param);
        for(i = 0; i < 2; i++) {
            if(!i) strncpy(buf, ksnet_getSysConfigDir(), KSN_BUFFER_SIZE);
            else strncpy(buf, getDataPath(), KSN_BUFFER_SIZE);
            if(conf->network[0]) {
                strncat(buf, "/", KSN_BUFFER_SIZE - strlen(buf) - 1);
                strncat(buf, conf->network, KSN_BUFFER_SIZE - strlen(buf) - 1);
            }
            strncat(buf, uconf, KSN_BUFFER_SIZE - strlen(buf) - 1);
            if(access(buf, F_OK) != -1 ) {
                cfg_parse(cfg, buf);
                save_conf_back();
    //            readPeersKey(conf, cfg);
    //            readRemoteCommand(cfg, buf);
            }
        }
        free(uconf);
    }

    //printf("setting username to 'foo'\n");
    /* using cfg_setstr here is not necessary at all, the equivalent
     * code is:
     *   free(username);
     *   username = strdup("foo");
     */
    //cfg_setstr(cfg, "user", "foo");
    //printf("username: %s\n", username);

    cfg_free(cfg);

    // Save file parameters for last use
    conf->pp = port_param;
    strncpy(conf->pn, conf->network, KSN_BUFFER_SM_SIZE);

    #undef save_conf_back
}

/**
 * Create unique port configuration file name
 *
 * @param buf Buffer to create file name
 * @param BUF_SIZE Size of buffer
 * @param type Configuration file type: 0 - system; 1 - local
 * @param port Port number
 * @param network Network name
 *
 * @return
 */
char* uconfigFileName(char *buf, const int BUF_SIZE, const int type,
              const int port,
              const char* network) {

    int i;
    char *uconf;

    if(port) uconf = ksnet_formatMessage("/teonet-%d.conf", port);
    else uconf = ksnet_formatMessage("/teonet.conf");
    for(i = 0; i < 2; i++) {
        if(i != type) continue;
        if(!i) strncpy(buf, ksnet_getSysConfigDir(), BUF_SIZE);
        else strncpy(buf, getDataPath(), BUF_SIZE);
        if(network != NULL && network[0]) {
            strncat(buf, "/", BUF_SIZE);
            strncat(buf, network, BUF_SIZE);
        }
        strncat(buf, uconf, BUF_SIZE);
    }
    free(uconf);

    return buf;
}

#if M_ENAMBE_VPN
/**
 * Add VPN Hardware address to configuration file
 *
 * @param hwaddr
 */
void ksnet_addHWAddrConfig(ksnet_cfg *conf, char *hwaddr) {

    // Open configuration file to append
    char buf[KSN_BUFFER_SM_SIZE];
    uconfigFileName(buf, KSN_BUFFER_SM_SIZE, 1, conf->pp, conf->pn);
    FILE *fp = fopen(buf, "a");

    // Write section
    fprintf(fp, "\nvpn_dev_hwaddr = %s\n", hwaddr);
    fclose(fp);
}
#endif
