/** 
 * File:   conf.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet web application configuration module
 * 
 * Created on November 29, 2015, 6:06 AM
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <confuse.h>

#include "conf.h"
#include "utils/utils.h"
#include "modules/teo_web/teo_web_conf.h"

//teoweb_config tw_cfg;

/**
 * Set default configuration parameters
 * 
 * @param tw_cfg
 */
static void teoweb_set_defaults(teoweb_config *tw_cfg) {
    
    tw_cfg->http_port = 8088;
    strncpy(tw_cfg->document_root, "/var/www", KSN_BUFFER_SM_SIZE);               
}

/**
 * Read configuration file
 * 
 * @param conf Pointer to teoweb_config
 * @param network Network name or NULL
 * @param port_param Teonet basic port
 */
void teowebConfigRead(teoweb_config *conf, const char *network, int port_param) {
    
    #define CONF_NAME "/teoweb.conf"
    // Save values back to structure
    #define save_conf_back() \
        strncpy(conf->document_root, document_root, KSN_BUFFER_SM_SIZE)

    // Load string values
    char *document_root = strdup(conf->document_root);
    
    // Config data define
    cfg_opt_t opts[] = {

        CFG_SIMPLE_STR("document_root", &document_root),
        CFG_SIMPLE_INT("http_port", &conf->http_port),
        
        CFG_END()
    };
    
    // Define config file name
    int i;
    cfg_t *cfg;
    cfg = cfg_init(opts, 0);
    char buf[KSN_BUFFER_SIZE];
    // Open and parse configure file in system and then in data directory
    for(i = 0; i < 2; i++) {
        
        // Parse parameters
        if(!i) strncpy(buf, ksnet_getSysConfigDir(), KSN_BUFFER_SIZE);
        else strncpy(buf, getDataPath(), KSN_BUFFER_SIZE);
        if(network != NULL && network[0]) {
            strncat(buf, "/", KSN_BUFFER_SIZE - strlen(buf) - 1);
            strncat(buf, network, KSN_BUFFER_SIZE - strlen(buf) - 1);
        }
        strncat(buf, CONF_NAME, KSN_BUFFER_SIZE - strlen(buf) - 1);
        if(access(buf, F_OK) != -1 ) {
            cfg_parse(cfg, buf);
            save_conf_back();
        }
        
        // Print the parsed values to save configuration file
        {
            strncpy(buf, getDataPath(), KSN_BUFFER_SIZE);
            if(network != NULL && network[0]) {
                strncat(buf, "/", KSN_BUFFER_SIZE - strlen(buf) - 1);
                strncat(buf, network, KSN_BUFFER_SIZE - strlen(buf) - 1);
            }
            strncat(buf, CONF_NAME".out", KSN_BUFFER_SIZE - strlen(buf) - 1);
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
        char *uconf = ksnet_formatMessage("/teoweb-%d.conf", port_param);
        for(i = 0; i < 2; i++) {
            if(!i) strncpy(buf, ksnet_getSysConfigDir(), KSN_BUFFER_SIZE);
            else strncpy(buf, getDataPath(), KSN_BUFFER_SIZE);
            if(network != NULL && network[0]) {
                strncat(buf, "/", KSN_BUFFER_SIZE - strlen(buf) - 1);
                strncat(buf, network, KSN_BUFFER_SIZE - strlen(buf) - 1);
            }
            strncat(buf, uconf, KSN_BUFFER_SIZE - strlen(buf) - 1);
            if(access(buf, F_OK) != -1 ) {
                cfg_parse(cfg, buf);
                save_conf_back();
            }
        }
        free(uconf);
    }
    
    cfg_free(cfg);

//    // Save file parameters for last use
//    conf->pp = port_param;
//    strncpy(conf->pn, conf->network, KSN_BUFFER_SM_SIZE);

    #undef save_conf_back    
    #undef CONF_NAME
}

/**
 * Read teoweb configuration file
 * 
 * @return Pointer to teoweb_config
 */
teoweb_config *teowebConfigInit() {
    
    teoweb_config *tw_cfg = malloc(sizeof(teoweb_config));
    memset(tw_cfg, 0, sizeof(teoweb_config));
    
    teoweb_set_defaults(tw_cfg);
            
    return tw_cfg;
}

/**
 * Free teoweb configuration
 */
void teowebConfigFree(teoweb_config *tw_cfg) {
    
    free(tw_cfg);
}
