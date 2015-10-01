/** 
 * File:   teo_http.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet HTTP/WS Server module
 *
 * Created on October 2, 2015, 12:04 AM
 */

#ifndef TEO_HTTP_H
#define	TEO_HTTP_H

#include "ev_mgr.h"
#include "embedded/mongoose/mongoose.h"

/**
 * HTTP module class data
 */
typedef struct ksnHTTPClass {
    
    void *ke; ///< Ponter to ksnetEvMgrClass
    char *s_http_port; ///< HTTP port
    struct mg_serve_http_opts s_http_server_opts; ///< HTTP server options
    pthread_t tid; ///< HTTP thread id
    int stop; ///< Stop HTTP server flag
    int stopped; ///< HTTP server is stopped
    
} ksnHTTPClass;

#ifdef	__cplusplus
extern "C" {
#endif

ksnHTTPClass* ksnHTTPInit(ksnetEvMgrClass *ke);
void ksnHTTPDestroy(ksnHTTPClass *kh);
void ksnHTTPEventCb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data);

#ifdef	__cplusplus
}
#endif

#endif	/* TEO_HTTP_H */

