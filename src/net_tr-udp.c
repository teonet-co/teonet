/** 
 * File:   net_tr-udp.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet Real time communications over UDP protocol (TR-UDP)
 *
 * Created on August 4, 2015, 12:16 AM
 */

#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "net_tr-udp.h"
#include "config/conf.h"
#include "utils/utils.h"
#include "utils/rlutil.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 0

#define kev ((ksnetEvMgrClass*)(((ksnCoreClass*)tu->kc)->ke))

/**
 * Initialize TR-UDP class
 * 
 * @param kc
 * @return 
 */
ksnTRUDPClass *ksnTRUDPInit(void *kc) {
    
    ksnTRUDPClass *tu = malloc(sizeof(ksnTRUDPClass));
    tu->kc = kc;
    tu->ip_map = pblMapNewHashMap();
            
    return tu;
}

/**
 * Destroy TR-UDP class
 * 
 * @param tu
 */
void ksnTRUDPDestroy(ksnTRUDPClass *tu) {
    
    if(tu != NULL) {
        ksnTRUDPSendListDestroyAll(tu);        
        free(tu);
    }
}

/**
 * TR-UDP message header structure
 */
typedef struct ksnTRUDP_header {
    
    unsigned int version_major : 4; ///< Protocol major version number
    unsigned int version_minor : 4; ///< Protocol minor version number
    /**
     * Message type could be of type DATA(0x0), ACK(0x1) and RESET(0x2).
     */
    uint8_t message_type; 
    /**
     * Payload length defines the number of bytes in the message payload
     */
    uint16_t payload_length; 
    /**
     * ID  is a message serial number that sender assigns to DATA and RESET 
     * messages. The ACK messages must copy the ID from the corresponding DATA 
     * and RESET messages. 
     */
    uint32_t id;
    /**
     * Timestamp (32 byte) contains sending time of DATA and RESET messages and 
     * filled in by message sender. The ACK messages must copy the timestamp 
     * from the corresponding DATA and RESET messages.
     */
    uint32_t timestamp; 
    
} ksnTRUDP_header;

/**
 * TR-UDP message type
 */
enum ksnTRUDP_type {
    
    tru_data, ///< The DATA messages are carrying payload. (has payload)
    /**
     * The ACK messages are used to acknowledge the arrival of the DATA and 
     * RESET messages. (has not payload) 
     */
    tru_ack, 
    tru_reset ///< The RESET messages reset messages counter. (has not payload)
            
};

/**
 * Send to peer through TR-UDP transport
 * 
 * @param fd
 * @param buf
 * @param buf_len
 * @param flags
 * @param addr
 * @param addr_len
 * 
 * @return 
 */
ssize_t ksnTRUDPsendto (ksnTRUDPClass *tu, int fd, int cmd, const void *buf, 
                        size_t buf_len, int flags, __CONST_SOCKADDR_ARG addr,
		        socklen_t addr_len) {
    
    ksnetEvMgrClass *ke = ((ksnCoreClass*)tu->kc)->ke;

    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
        "%sTR-UDP:%s got %d bytes data to send, cmd %d\n", 
        ANSI_LIGHTGREEN, ANSI_NONE,
        buf_len, cmd
    );
    #endif
    
    // Check commands array
    if(!inarray(cmd, not_RTUDP, not_RTUDP_len)) {
    
        // TR-UDP packet buffer
        const size_t tru_ptr = sizeof(ksnTRUDP_header); // Header size
        char tru_buf[KSN_BUFFER_DB_SIZE]; // Packet buffer
        ksnTRUDP_header tru_header; // Header buffer

        // TODO: If payload_length more than buffer crop data or drop this packet
        if(buf_len + tru_ptr > KSN_BUFFER_DB_SIZE) {        
            buf_len = KSN_BUFFER_DB_SIZE - tru_ptr;
        }

        // Make TR-UDP Header
        tru_header.version_major = VERSION_MAJOR;
        tru_header.version_minor = VERSION_MINOR;
        tru_header.message_type = tru_data;
        tru_header.payload_length = buf_len;
        tru_header.timestamp = 0; 
        
        // Get new message ID
        tru_header.id = ksnTRUDPSendListNewID(tu, addr, addr_len);

        // Copy TR-UDP header
        memcpy(tru_buf, &tru_header, tru_ptr);

        // Copy TR-UDP message
        memcpy(tru_buf + tru_ptr, buf, buf_len);

        // New buffer length
        buf_len += tru_ptr;
        buf = tru_buf;

        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
            "%sTR-UDP:%s >> send %d bytes message of type: %d "
            "with %d bytes data payload to %s:%d\n", 
            ANSI_LIGHTGREEN, ANSI_NONE,
            buf_len, tru_header.message_type, 
            tru_header.payload_length,
            inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),
            ntohs(((struct sockaddr_in *)addr)->sin_port)
        );
        #endif

        // Add packet to Sent message list (Acknowledge Pending Messages)
        ksnTRUDPSendListAdd(tu, tru_header.id, fd, cmd, buf, buf_len, flags, 
                addr, addr_len);
    } 
    else {
        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
            "%sTR-UDP:%s >> skip this packet, "
            "send %d bytes direct by UDP to: %s:%d\n", 
            ANSI_LIGHTGREEN, ANSI_NONE,
            buf_len, 
            inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),
            ntohs(((struct sockaddr_in *)addr)->sin_port)
        );
        #endif
    }

    return sendto(fd, buf, buf_len, flags, addr, addr_len);
}

/**
 * Get data from peer through TR-UDP transport
 * 
 * @param fd
 * @param buf
 * @param buf_len
 * @param flags
 * @param addr
 * @param addr_len
 * @return 
 */
ssize_t ksnTRUDPrecvfrom (ksnTRUDPClass *tu, int fd, void *buf, size_t buf_len, 
                          int flags, __SOCKADDR_ARG addr, socklen_t *addr_len) {
    
    const size_t tru_ptr = sizeof(ksnTRUDP_header); // Header size
    ksnetEvMgrClass *ke = ((ksnCoreClass*)tu->kc)->ke;
                
    // Get data
    ssize_t recvlen = recvfrom (fd, buf, buf_len, flags, addr, addr_len);
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
        "%sTR-UDP:%s << got %d bytes packet, from %s:%d\n", 
        ANSI_LIGHTGREEN, ANSI_NONE, 
        recvlen, inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),
        ntohs(((struct sockaddr_in *)addr)->sin_port)
    );
    #endif
    
    // Data received
    if(recvlen > 0) {
        
        ksnTRUDP_header *tru_header = buf;
        
        // Check for TR-UDP header
        if(recvlen - tru_ptr == tru_header->payload_length) {
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
                "%sTR-UDP:%s process %d bytes message of type %d, "
                "with %d bytes data payload\n", 
                ANSI_LIGHTGREEN, ANSI_NONE, 
                recvlen, tru_header->message_type, tru_header->payload_length
            );
            #endif
            
            switch(tru_header->message_type) {
                
                // The DATA messages are carrying payload. (has payload)
                // Process this message.
                // Return message with payload if there is expected message or 
                // zero len message if this message was push to queue.
                case tru_data:
                {
                    // Send ACK to sender
                    ksnTRUDP_header tru_send_header;
                    memcpy(&tru_send_header, tru_header, tru_ptr);
                    tru_send_header.payload_length = 0;
                    tru_send_header.message_type = tru_ack;
                    sendto(fd, &tru_send_header, tru_ptr, 0, addr, *addr_len);
                    
                    // Extract message from TR-UDP buffer
                    recvlen = tru_header->payload_length;
                    if(recvlen > 0) memmove(buf, buf + tru_ptr, recvlen);
                    
                    // TODO: If Message ID Equals to Expected ID send to core or 
                    // save to message Heap sorted by ID
                    // ksnTRUDPReceiveHeapAdd();                    
                    
                }   break;
                
                // The ACK messages are used to acknowledge the arrival of the 
                // DATA and RESET messages. (has not payload)
                // Return zero length of this message.    
                case tru_ack:
                    
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
                        "%sTR-UDP:%s +++ got ACK to message ID %d\n", 
                        ANSI_LIGHTGREEN, ANSI_NONE, 
                        tru_header->id
                    );
                    #endif

                    // Remove message from SendList
                    ksnTRUDPSendListRemove(tu, tru_header->id, addr, *addr_len);
                    recvlen = 0;
                    break;
                    
                // The RESET messages reset messages counter. (has not payload)  
                // Return zero length of this message.    
                case tru_reset:
                    
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
                        "%sTR-UDP:%s +++ got RESET command\n", 
                        ANSI_LIGHTGREEN, ANSI_NONE
                    );
                    #endif

                    // TODO: Process RESET command
                    recvlen = 0;
                    break;
                    
                // Some error or undefined message. Don't process this message.
                // Return this message without any changes.    
                default:
                    break;
            }
        }
        else {
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
                "%sTR-UDP:%s skip received packet\n", 
                ANSI_LIGHTGREEN, ANSI_NONE
            );
            #endif
        }
    }
    
    return recvlen;
}

/*****************************************************************************
 *
 *  Send list
 * 
 *****************************************************************************/

/**
 * Create key from address
 * 
 * @param addr
 * @param key
 * @param key_len
 * @return 
 */
size_t ksnTRUDPSendListGetKey( __CONST_SOCKADDR_ARG addr, char* key, size_t key_len) {
    
    return snprintf(key, key_len, "%s:%d", 
            inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),
            ntohs(((struct sockaddr_in *)addr)->sin_port) );
}

/**
 * Remove message from Send message List (Acknowledge Pending Messages)
 * 
 * @param tu
 * @param id
 * @param addr
 * @param addr_len
 * @return 
 */
int ksnTRUDPSendListRemove(ksnTRUDPClass *tu, int id, __CONST_SOCKADDR_ARG addr, 
        socklen_t addr_len) {

    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPSendListGetKey(addr, key, KSN_BUFFER_SM_SIZE);
    PblMap **sl = pblMapGet(tu->ip_map, key, key_len, &val_len);
    if(sl != NULL) {
        
        pblMapRemove(*sl, &id, sizeof(id), &val_len);
        
        //ksnetEvMgrClass *ke = ((ksnCoreClass*)tu->kc)->ke;       
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
            "%sTR-UDP:%s message with id %d was removed from %s Send List "
            "(len %d)\n", 
            ANSI_LIGHTGREEN, ANSI_NONE,
            id, key, pblMapSize(*sl)
        );
        #endif        
    }
    
    return 1;
}

/**
 * Send List timer data structure
 */
typedef struct sl_timer_cb_data {

    ksnTRUDPClass *tu;
    int id;
    int fd;
    int cmd;
    int flags;
    PblMap *sl;
    __CONST_SOCKADDR_ARG addr;
    socklen_t addr_len;

} sl_timer_cb_data;

/**
 * Send list timer callback
 * 
 * @param loop
 * @param w
 * @param revents
 */
void sl_timer_cb(EV_P_ ev_timer *w, int revents) {
    
    sl_timer_cb_data *sl_data = w->data;
    ksnTRUDPClass *tu = sl_data->tu;

    // Stop this timer
    ev_timer_stop(EV_A_ w);

    // Get message from list
    size_t data_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPSendListGetKey(sl_data->addr, key, KSN_BUFFER_SM_SIZE);
    void *data = pblMapGet(sl_data->sl, key, key_len, &data_len);
    
    if(data != NULL) {
        
        // Resend message
        ksnTRUDPsendto(tu, sl_data->fd, sl_data->cmd, data, data_len, 
                           sl_data->flags, sl_data->addr, sl_data->addr_len);

        // Remove record from list
        ksnTRUDPSendListRemove(tu, sl_data->id, sl_data->addr, sl_data->addr_len);
    }
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
      "%sTR-UDP:%s timeout for message with id %d was happened\n", 
      ANSI_LIGHTGREEN, ANSI_NONE,
      sl_data->id
    );
    #endif
   
    // Free watcher and its data
    free(w->data);
    free(w);
}

/**
 * Add packet to Sent message list (Acknowledge Pending Messages)
 * 
 * @param tu
 * @param id
 * @param fd
 * @param cmd
 * @param data
 * @param data_len
 * @param flags
 * @param addr
 * @param addr_len
 * 
 * @return 
 */
int ksnTRUDPSendListAdd(ksnTRUDPClass *tu, int id, int fd, int cmd, 
        const void *data, size_t data_len, int flags, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {
       
    // Get Send List by address from ip_map
    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPSendListGetKey(addr, key, KSN_BUFFER_SM_SIZE);
    PblMap **sl = pblMapGet(tu->ip_map, key, key_len, &val_len);
    
   // Create new Send List if it not exist
    if(sl == NULL) {
        
        PblMap *sendl = pblMapNewHashMap();
        pblMapAdd(tu->ip_map, key, key_len, &sendl, sizeof(PblMap *));
        sl = &sendl;
    }
    
    // Create ACK timeout timer
    ev_timer *w = malloc(sizeof(ev_timer));
    ev_timer_init(w, sl_timer_cb, 0.0, 2.0);    
    sl_timer_cb_data *sl_data = malloc(sizeof(sl_timer_cb_data));
    sl_data->tu = tu;
    sl_data->sl = *sl;
    sl_data->id = id;
    sl_data->fd = fd;
    sl_data->cmd = cmd;
    sl_data->flags = flags;
    sl_data->addr = addr;
    sl_data->addr_len = addr_len;
    w->data = sl_data;
    ev_timer_start(kev->ev_loop, w);
    
    // Add message to Send List
    pblMapAdd(*sl, &id, sizeof(id), (void*)data, data_len);
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
      "%sTR-UDP:%s message with id %d was added to %s Send List (len %d)\n", 
      ANSI_LIGHTGREEN, ANSI_NONE,
      id, key, pblMapSize(*sl)
    );
    #endif
    
    return 1;
}

// TODO: Get new message ID
int ksnTRUDPSendListNewID(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr, 
        socklen_t addr_len) {
    
    static uint32_t msg_id = 0;

    return msg_id++;
}

/**
 * Destroy all Sent message lists
 * 
 * @param tu
 * @return 
 */
int ksnTRUDPSendListDestroyAll(ksnTRUDPClass *tu) {

    // TODO: Clear all Send List
    
    pblMapFree(tu->ip_map);
    
    return 0;
}
