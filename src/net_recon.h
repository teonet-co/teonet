/** 
 * \file   net_recon.h
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet reconnect module.
 * 
 *
 * Created on November 5, 2015, 11:15 AM
 */

#ifndef NET_RECON_H
#define	NET_RECON_H

#include <pbl.h>
#include "net_com.h"
#include "modules/cque.h"

typedef struct ksnReconnectClass ksnReconnectClass;

/**
 * ksnReconnectClass definition
 */
struct ksnReconnectClass {
    
    ksnReconnectClass *this; ///< Pointer to this class
    void *kco; ///< Pointer to ksnCommandClass
    PblMap* map; ///< Hash Map to store reconnect requests
    
    // Public methods
    
    /**
     * Send CMD_RECONNECT command to r-host
     * 
     * @param this Pointer to ksnReconnectClass
     * @param peer Peer name to reconnect
     * @return Zero if success or error
     * @retval  0 Send command successfully
     * @retval -1 Has not connected to r-host
     */
    int (*send)(ksnReconnectClass *rec, const char *peer); 
    
    /**
     * Send CMD_RECONNECT_ANSWER command to r-host
     * 
     * @param this Pointer to ksnReconnectClass
     * @param peer Peer name to send
     * @param peer_to_reconnect Peer name to reconnect
     * @return Zero if success or error
     * @retval  0 Send command successfully
     * @retval -1 Has not connected to peer
     * @retval -2 This class has not initialized yet
     */
    int (*sendAnswer)(ksnReconnectClass *rec, const char *peer, 
            const char* peer_to_reconnect); 
    
    /**
     * Process CMD_RECONNECT command 
     * 
     * Got by r-host from peer wanted reconnect with peer rd->data
     * 
     * @param this Pointer to ksnReconnectClass
     * @param rd Pointer to ksnCorePacketData
     * @return 
     */    
    int (*process)(ksnReconnectClass *rec, ksnCorePacketData *rd);
    
    /**
     * Process CMD_RECONNECT_ANSWER command 
     * 
     * Got from r-host if peer to reconnect has not connected to r-host
     * 
     * @param this Pointer to ksnReconnectClass
     * @param rd Pointer to ksnCorePacketData
     * @return 
     */
    int (*processAnswer)(ksnReconnectClass *rec, ksnCorePacketData *rd);
    
    /**
     * CallbackQueue callback
     * 
     * @param id Calls ID
     * @param type Type: 0 - timeout callback; 1 - successful callback 
     * @param data User data selected in ksnCQueAdd function
     *  
     */
    ksnCQueCallback callback; 
    
    /**
     * Destroy ksnReconnectClass
     * 
     * @param this Pointer to ksnReconnectClass
     */
    void (*destroy)(ksnReconnectClass *rec); 
    
};

#ifdef	__cplusplus
extern "C" {
#endif

ksnReconnectClass *ksnReconnectInit(void *kco);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_RECON_H */
