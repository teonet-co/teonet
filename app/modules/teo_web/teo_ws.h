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
/**
 * Websocket L0 connector class data
 */
struct teoWSClass {
    ksnHTTPClass *kh; ///< Pointer to ksnHTTPClass
    PblMap* map; ///< Hash Map to store websocket clients
    
    // Public methods
    
    /**
     * Destroy teonet HTTP module]
     * @param kws Pointer to teoWSClass
     */
    void (*destroy)(teoWSClass *kws); 
    
    /**
     * Connect WS client with L0 server and add it to connected map and create 
     * READ watcher
     * 
     * @param kws Pointer to teoWSClass
     * @param nc_p Pointer to websocket connector
     * @param server L0 Server name or IP
     * @param port L0 Server port
     * @param login L0 server login
     * 
     * @return Pointer to teoLNullConnectData or NULL if error
     */    
    teoLNullConnectData * (*add)(teoWSClass *kws, void *nc_p, 
                            const char *server, const int port, char *login);
    
    /*
     * Disconnect WS client from L0 server, remove it from connected map and stop 
     * READ watcher
     * 
     * @param kws Pointer to teoWSClass
     * @param nc_p Pointer to mg_connection structure
     * 
     * @return Return true at success
     */    
    int (*remove)(teoWSClass *kws, void *nc_p);
    
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
     * Send command to L0 server
     * 
     * Create L0 clients packet and send it to L0 server
     * 
     * @param con Pointer to teoLNullConnectData
     * @param cmd Command
     * @param peer_name Peer name to send to
     * @param data Pointer to data
     * @param data_length Length of data
     * 
     * @return Length of send data or -1 at error
     */
    ssize_t (*send)(teoWSClass *kws, void *nc_p, int cmd, 
        const char *to_peer_name, void *data, size_t data_length);
    
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

/**
 * Websocket L0 connector map data
 */
typedef struct teoWSmapData {
    teoLNullConnectData *con; ///< Pointer to L0 client connect data
    ev_io w; ///< L0 client watcher
} teoWSmapData;

#ifdef	__cplusplus
extern "C" {
#endif

teoWSClass* teoWSInit(ksnHTTPClass *kh);


#ifdef	__cplusplus
}
#endif

#endif	/* TEO_WS_H */

