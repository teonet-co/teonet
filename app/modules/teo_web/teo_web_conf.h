/** 
 * File:   conf.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet web application configuration module
 * 
 * Created on November 29, 2015, 6:04 AM
 */

#ifndef TEOWEB_CONF_H
#define	TEOWEB_CONF_H

/**
 * Teoweb configuration structure
 */
typedef struct teoweb_config {

    long http_port;
    char document_root[KSN_BUFFER_SM_SIZE];
    
//    char *name;
//    char *web_site;
//    char *server;
//    char *database;
//    char *login;
//    char *password;

} teoweb_config;

#ifdef	__cplusplus
extern "C" {
#endif

    
teoweb_config *teowebConfigInit();
void teowebConfigRead(teoweb_config *conf, const char *network, int port_param);
void teowebConfigFree();


#ifdef	__cplusplus
}
#endif

#endif	/* TEOWEB_CONF_H */
