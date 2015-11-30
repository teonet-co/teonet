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

#include "teo_web/teo_web_conf.h"

//teoweb_config tw_cfg;

/**
 * Set default configuration parameters
 * 
 * @param tw_cfg
 */
static void set_defaults(teoweb_config *tw_cfg) {
    
    tw_cfg->http_port = 8088;
    tw_cfg->document_root = "/var/www";               
}

/**
 * Read teoweb configuration file
 * 
 * @return Pointer to teoweb_config
 */
teoweb_config *teowebConfigRead() {
    
    teoweb_config *tw_cfg = malloc(sizeof(teoweb_config));
    memset(tw_cfg, 0, sizeof(teoweb_config));
    
    set_defaults(tw_cfg);
            
    return tw_cfg;
}

/**
 * Free teoweb configuration
 */
void teowebConfigFree(teoweb_config *tw_cfg) {
    
    free(tw_cfg);
}
