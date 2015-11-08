/** 
 * File:   teo_ws.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet websocket L0 connector module
 *
 * Created on November 8, 2015, 3:57 PM
 */

#ifndef TEO_WS_H
#define	TEO_WS_H

#include "teo_web.h"

typedef struct teoWSClass teoWSClass;

struct teoWSClass {
    
    ksnHTTPClass *kh;
    
    // Public methods
    
    /**
     * Destroy teonet HTTP module]
     * @param kws Pointer to teoWSClass
     */
    void (*destroy)(teoWSClass *kws); 
    
    /**
     * Teonet L0 websocket event handler
     * 
     * @param kws Pointer to teoWSClass
     * @param ev Event
     * @param nc_p Pointer to mg_connection structure
     * @param data Websocket data
     * @param data_length Websocket data length
     * 
     * @return True if event processed
     */    
    int (*handler)(teoWSClass *kws, int ev, void *nc_p, void *data, size_t data_length);
    
    /**
     * Process websocket message
     * 
     * @param kws Pointer to teoWSClass
     * @param nc_p Pointer to mg_connection structure
     * @param data Websocket data
     * @param data_length Websocket data length
     * 
     * @return  True if message processed
     */    
    int (*processMsg)(teoWSClass *kws, void *nc_p, void *data, size_t data_length);

};

#ifdef	__cplusplus
extern "C" {
#endif

teoWSClass* teoWSInit(ksnHTTPClass *kh);


#ifdef	__cplusplus
}
#endif

#endif	/* TEO_WS_H */

