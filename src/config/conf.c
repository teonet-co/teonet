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

extern const char *localhost, *null_str;

// Local function
void set_defaults(teonet_cfg *teo_cfg);

/**
 * Get configuration parameters
 */
void ksnet_configInit(teonet_cfg *teo_cfg, void *ke) {

    teo_cfg->ke = ke;
    set_defaults(teo_cfg);
    //! \todo: Set port param
    //int port_param = 0;
    //read_config(teo_cfg, port_param);
}

/**
 * Set default configuration parameters
 */
void set_defaults(teonet_cfg *teo_cfg) {

    // Encrypt/Decrypt packets
    teo_cfg->crypt_f = KSNET_CRYPT;

    strncpy(teo_cfg->network, "local", KSN_BUFFER_SM_SIZE/2);
    teo_cfg->net_key[0] = '\0';
    teo_cfg->auth_secret[0] = '\0';

    // Show info at console flags
    teo_cfg->show_connect_f = 1;
    teo_cfg->show_debug_f = 1;
    teo_cfg->show_debug_vv_f = 0;
    teo_cfg->show_debug_vvv_f = 0;
    teo_cfg->show_peers_f = 0;
    teo_cfg->show_tr_udp_f = 0;
    
    // Other flags
    teo_cfg->send_ack_event_f = 0;
    teo_cfg->block_cli_input_f = 0;
    teo_cfg->no_multi_thread_f = 0;

    // This host
    teo_cfg->port = atoi(KSNET_PORT_DEFAULT);
    teo_cfg->port_inc_f = 1;
    char *name = getRandomHostName();
    strncpy(teo_cfg->host_name, name, KSN_MAX_HOST_NAME);
    free(name);
    
    // TCP Proxy
    teo_cfg->tcp_allow_f = 0;
    teo_cfg->tcp_port = teo_cfg->port;
    
    // L0 Server
    teo_cfg->l0_allow_f = 0;
    teo_cfg->l0_tcp_port = teo_cfg->port;
    teo_cfg->l0_tcp_ip_remote[0] = '\0';
    
    // Display log filter
    teo_cfg->filter[0] = '\0';
            
    // Remote host default
    teo_cfg->r_port = atoi(KSNET_PORT_DEFAULT);
    teo_cfg->r_host_name[0] = '\0';
    teo_cfg->r_host_addr[0] = '\0';
    teo_cfg->r_host_addr_opt[0] = '\0';
    teo_cfg->r_tcp_f = 0;
    teo_cfg->r_tcp_port = atoi(KSNET_PORT_DEFAULT);

    // Hosts public IPs
    teo_cfg->l0_public_ipv4[0] = '\0';
    teo_cfg->l0_public_ipv6[0] = '\0';

    // VPN
    teo_cfg->vpn_dev_name[0] = '\0';
    teo_cfg->vpn_dev_hwaddr[0] = '\0';
    teo_cfg->vpn_ip[0] = '\0';
    teo_cfg->vpn_ip_net = 24;
    teo_cfg->vpn_connect_f = 0;
    teo_cfg->vpn_mtu = 0;
    
    // Logging server
    teo_cfg->logging_f = 0;
    
    // Disable send logs to logging server
    teo_cfg->log_disable_f = 0;
    
    // Disable color terminal output
    teo_cfg->color_output_disable_f = 0;

    // Extended L0 log
    teo_cfg->extended_l0_log_f = 0;
    
    // SIGSEGV processing
    teo_cfg->sig_segv_f = 0;
    
    // Syslog priority
    teo_cfg->log_priority = DEBUG;

    // Send peers metric flag
    teo_cfg->statsd_peers_f = 0;
    

    // Create prefix
    const char* LOG_PREFIX = "teonet:";
    const size_t LOG_PREFIX_SIZE = strlen(LOG_PREFIX);
    size_t prefix_len = LOG_PREFIX_SIZE + strlen(teo_cfg->app_name) + 1;
    //teo_cfg->log_prefix = malloc(prefix_len); // \todo Free this at exit
    strncpy(teo_cfg->log_prefix, LOG_PREFIX, prefix_len);
    strncat(teo_cfg->log_prefix, teo_cfg->app_name, prefix_len - LOG_PREFIX_SIZE);

    // Terminal
//    strncpy(teo_cfg->t_username, "fred", KSN_BUFFER_SM_SIZE/2);
//    strncpy(teo_cfg->t_password, "nerk", KSN_BUFFER_SM_SIZE/2);
}

/**
 * Read configuration parameters from file
 * 
 * @param conf
 * @param port_param
 */
void read_config(teonet_cfg *conf, int port_param) {

    // Save values back to structure
    #define save_conf_back() \
        strncpy(conf->net_key, net_key, KSN_BUFFER_SM_SIZE/2); \
        strncpy(conf->auth_secret, auth_secret, KSN_BUFFER_SM_SIZE/2); \
        strncpy(conf->host_name, host_name, KSN_MAX_HOST_NAME); \
        strncpy(conf->r_host_addr, r_host_addr, KSN_BUFFER_SM_SIZE/2); \
        strncpy(conf->vpn_ip, vpn_ip, KSN_MAX_HOST_NAME); \
        strncpy(conf->statsd_ip, statsd_ip, KSN_BUFFER_SM_SIZE/2); \
        strncpy(conf->vpn_dev_name, vpn_dev_name, KSN_MAX_HOST_NAME); \
        strncpy(conf->vpn_dev_hwaddr, vpn_dev_hwaddr, KSN_MAX_HOST_NAME); \
        strncpy(conf->l0_tcp_ip_remote, l0_tcp_ip_remote, KSN_BUFFER_SM_SIZE/2); \
        strncpy(conf->filter, filter, KSN_BUFFER_SM_SIZE/2); \
        strncpy(conf->l0_public_ipv4, l0_public_ipv4, KSN_BUFFER_SM_SIZE/2); \
        strncpy(conf->l0_public_ipv6, l0_public_ipv6, KSN_BUFFER_SM_SIZE/2)

    // Load string values
    char *vpn_ip = strdup(conf->vpn_ip);
    char *filter = strdup(conf->filter);
    char *net_key = strdup(conf->net_key);
    char *auth_secret = strdup(conf->auth_secret);
    char *statsd_ip = strdup(conf->statsd_ip);
    char *host_name = strdup(conf->host_name);
    char *r_host_addr = strdup(conf->r_host_addr);
    char *vpn_dev_name = strdup(conf->vpn_dev_name);
    char *vpn_dev_hwaddr = strdup(conf->vpn_dev_hwaddr);
    char *l0_tcp_ip_remote = strdup(conf->l0_tcp_ip_remote);
    char *l0_public_ipv4 = strdup(conf->l0_public_ipv4);
    char *l0_public_ipv6 = strdup(conf->l0_public_ipv6);

    char *config_dir = ksnet_getSysConfigDir();
    char *data_path = getDataPath();

    cfg_opt_t opts[] = {

        CFG_SIMPLE_STR("host_name", &host_name),
        CFG_SIMPLE_INT("port", &conf->port),
        CFG_SIMPLE_BOOL("port_inc_f", (cfg_bool_t*)&conf->port_inc_f),
        
        CFG_SIMPLE_STR("key", &net_key),
        CFG_SIMPLE_STR("auth_secret", &auth_secret),
        
        CFG_SIMPLE_INT("tcp_port", &conf->tcp_port),
        CFG_SIMPLE_BOOL("tcp_allow_f", (cfg_bool_t*)&conf->tcp_allow_f),

        CFG_SIMPLE_BOOL("l0_allow_f", (cfg_bool_t*)&conf->l0_allow_f),
        CFG_SIMPLE_INT("l0_tcp_port", &conf->l0_tcp_port),
        CFG_SIMPLE_STR("l0_tcp_ip_remote", &l0_tcp_ip_remote),

        CFG_SIMPLE_STR("filter", &filter),
        
        CFG_SIMPLE_STR("r_host_addr", &r_host_addr),
        CFG_SIMPLE_INT("r_port", &conf->r_port),
        
        CFG_SIMPLE_BOOL("r_tcp_f", (cfg_bool_t*)&conf->r_tcp_f),
        CFG_SIMPLE_INT("r_tcp_port", &conf->r_tcp_port),        

        #if KSNET_CRYPT
        CFG_SIMPLE_BOOL("crypt_f", (cfg_bool_t*)&conf->crypt_f),
        #endif

        CFG_SIMPLE_BOOL("show_connect_f", (cfg_bool_t*)&conf->show_connect_f),
        CFG_SIMPLE_BOOL("show_debug_f", (cfg_bool_t*)&conf->show_debug_f),
        CFG_SIMPLE_BOOL("show_debug_vv_f", (cfg_bool_t*)&conf->show_debug_vv_f),
        CFG_SIMPLE_BOOL("show_debug_vvv_f", (cfg_bool_t*)&conf->show_debug_vvv_f),
        CFG_SIMPLE_BOOL("show_peers_f", (cfg_bool_t*)&conf->show_peers_f),
        CFG_SIMPLE_BOOL("show_tr_udp_f", (cfg_bool_t*)&conf->show_tr_udp_f),
        CFG_SIMPLE_BOOL("hot_keys_f", (cfg_bool_t*)&conf->hot_keys_f),
        CFG_SIMPLE_BOOL("daemon_mode_f", (cfg_bool_t*)&conf->dflag),
        CFG_SIMPLE_BOOL("sig_segv_f", (cfg_bool_t*)&conf->sig_segv_f),
        CFG_SIMPLE_BOOL("block_cli_input_f", (cfg_bool_t*)&conf->block_cli_input_f),
        CFG_SIMPLE_BOOL("no_multi_thread_f", (cfg_bool_t*)&conf->no_multi_thread_f),
        CFG_SIMPLE_BOOL("send_ack_event_f", (cfg_bool_t*)&conf->send_ack_event_f),

        #if M_ENAMBE_VPN
        CFG_SIMPLE_BOOL("vpn_connect_f", (cfg_bool_t*)&conf->vpn_connect_f),
        CFG_SIMPLE_STR("vpn_ip", &vpn_ip),
        CFG_SIMPLE_INT("vpn_ip_net", &conf->vpn_ip_net),
        CFG_SIMPLE_INT("vpn_mtu", &conf->vpn_mtu),
        CFG_SIMPLE_STR("vpn_dev_name", &vpn_dev_name),
        CFG_SIMPLE_STR("vpn_dev_hwaddr", &vpn_dev_hwaddr),
        #endif

        #if M_ENAMBE_LOGGING_SERVER
        CFG_SIMPLE_BOOL("logging_f", (cfg_bool_t*)&conf->logging_f),
        #endif

        #if M_ENAMBE_LOGGING_CLIENT
        CFG_SIMPLE_BOOL("log_disable_f", (cfg_bool_t*)&conf->log_disable_f),
        CFG_SIMPLE_BOOL("send_all_logs_f", (cfg_bool_t*)&conf->send_all_logs_f),        
        #endif

        CFG_SIMPLE_INT("log_priority", &conf->log_priority),

        CFG_SIMPLE_BOOL("color_output_disable_f", (cfg_bool_t*)&conf->color_output_disable_f),
        CFG_SIMPLE_BOOL("extended_l0_log_f", (cfg_bool_t*)&conf->extended_l0_log_f),

        CFG_SIMPLE_STR("l0_public_ipv4", &l0_public_ipv4),
        CFG_SIMPLE_STR("l0_public_ipv6", &l0_public_ipv6),

        CFG_SIMPLE_STR("statsd_ip", &statsd_ip),
        CFG_SIMPLE_INT("statsd_port", &conf->statsd_port),
        CFG_SIMPLE_BOOL("statsd_peers_f", (cfg_bool_t*)&conf->statsd_peers_f),

        CFG_END()
    };

    int i;
    cfg_t *cfg;
    cfg = cfg_init(opts, 0);
    char buf[KSN_BUFFER_SIZE];

    // Open and parse common configure file in system and then in data directory
    for(i = 0; i < 2; i++) {

        if(!i) {
            strncpy(buf, config_dir, KSN_BUFFER_SIZE);
        } else {
            strncpy(buf, data_path, KSN_BUFFER_SIZE);
        }

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
            strncpy(buf, data_path, KSN_BUFFER_SIZE);

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
            if(!i) {
                strncpy(buf, config_dir, KSN_BUFFER_SIZE);
            } else {
                strncpy(buf, data_path, KSN_BUFFER_SIZE);
            }
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
    free(l0_public_ipv4);
    free(l0_public_ipv6);
    free(l0_tcp_ip_remote);
    free(vpn_dev_hwaddr);
    free(vpn_dev_name);
    free(r_host_addr);
    free(host_name);
    free(statsd_ip);
    free(net_key);
    free(auth_secret);
    free(filter);
    free(vpn_ip);

    free(data_path);
    free(config_dir);

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
    char *config_dir = ksnet_getSysConfigDir();
    char *data_path = getDataPath();

    if(port) uconf = ksnet_formatMessage("/teonet-%d.conf", port);
    else uconf = ksnet_formatMessage("/teonet.conf");
    for(i = 0; i < 2; i++) {
        if(i != type) continue;

        if(!i) {
            strncpy(buf, config_dir, BUF_SIZE);
        } else {
            strncpy(buf, data_path, BUF_SIZE);
        }

        if(network != NULL && network[0]) {
            strncat(buf, "/", BUF_SIZE);
            strncat(buf, network, BUF_SIZE);
        }
        strncat(buf, uconf, BUF_SIZE);
    }
    free(uconf);

    free(data_path);
    free(config_dir);

    return buf;
}

#if M_ENAMBE_VPN
/**
 * Add VPN Hardware address to configuration file
 *
 * @param hwaddr
 */
void ksnet_addHWAddrConfig(teonet_cfg *conf, char *hwaddr) {

    // Open configuration file to append
    char buf[KSN_BUFFER_SM_SIZE];
    uconfigFileName(buf, KSN_BUFFER_SM_SIZE, 1, conf->pp, conf->pn);
    FILE *fp = fopen(buf, "a");

    // Write section
    fprintf(fp, "\nvpn_dev_hwaddr = %s\n", hwaddr);
    fclose(fp);
}
#endif
