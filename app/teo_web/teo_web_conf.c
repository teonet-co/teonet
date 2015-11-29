/** 
 * File:   conf.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet web application configuration module
 * 
 * Created on November 29, 2015, 6:06 AM
 */

#include <string.h>

#include "teo_web/teo_web_conf.h"

teoweb_config tw_cfg;

/**
 * Read teoweb configuration file
 * 
 * @return Pointer to teoweb_config
 */
teoweb_config *teowebConfigRead() {
    
    memset(&tw_cfg, 0, sizeof(tw_cfg));
    
    tw_cfg.http_port = 8088;
    tw_cfg.document_root = "/var/www";
            
    return &tw_cfg;
}

/**
 * Get teoweb configuration
 * 
 * @return Pointer to teoweb_config
 */
teoweb_config *teowebConfigGet() {

    return &tw_cfg;
}

/**
 * Free teoweb configuration
 */
void teowebConfigFree() {
    
}
