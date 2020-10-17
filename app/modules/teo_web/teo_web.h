/** 
 * File:   teo_web.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet HTTP/WS Server module
 *
 * Created on October 2, 2015, 12:04 AM
 */

#ifndef TEO_HTTP_H
#define	TEO_HTTP_H

#include "ev_mgr.h"
#include "teo_web_conf.h"
//#include "../teo_auth/teo_auth.h"
#include "embedded/mongoose/mongoose.h"

/**
 * HTTP module class data
 */
typedef struct ksnHTTPClass {
    
    void *ke; ///< Ponter to ksnetEvMgrClass
    void *kws; ///< Pointer to teoWSClass
    void *ta; ///< Pointer to teoAuthClass
    teoweb_config *conf; ///< Pointer to teoweb_config
    char *s_http_port; ///< HTTP port
    struct mg_serve_http_opts s_http_server_opts; ///< HTTP server options
    pthread_t tid; ///< HTTP thread id
    int stop; ///< Stop HTTP server flag
    int stopped; ///< HTTP server is stopped
    
} ksnHTTPClass;

/**
 * Teonet HTTP module async event commands
 */
enum WS_CMD {
    
    WS_CONNECTED, ///< Websocket connected
    WS_MESSAGE, ///< Websocket send message
    WS_DISCONNECTED ///< Websocket disconnected
            
};

/**
 * Teonet HTTP module async data structure
 */
struct teoweb_data {
    
    uint16_t cmd; ///< Teonet HTTP module async event commands
    size_t data_len; ///< Legth of websocket data
    char data[]; ///< Websocket data
};

#ifdef	__cplusplus
extern "C" {
#endif

ksnHTTPClass* ksnHTTPInit(ksnetEvMgrClass *ke, teoweb_config *tw_cfg);
void ksnHTTPDestroy(ksnHTTPClass *kh);

#ifdef	__cplusplus
}
#endif

#endif	/* TEO_HTTP_H */
