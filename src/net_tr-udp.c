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
#define MAX_ACK_WAIT 2.0


/**
 * IP map records data
 */
typedef struct ip_map_data {
    
    uint32_t id; ///< Send message ID 
    uint32_t expected_id; ///< Receive message expected ID 
    PblMap *send_list; ///< Send messages list
    PblHeap *receive_heap; ///< Received messages heap
    
} ip_map_data;

/**
 * Send list data structure
 */
typedef struct sl_data {
    
    ev_timer *w;
    void *data;
    size_t data_len;
  
} sl_data;

/**
 * Receive heap data
 */
typedef struct rh_data {
    
    uint32_t id;
    void *data;
    size_t data_len;
    
} rh_data;

/**
 * Send List timer data structure
 */
typedef struct sl_timer_cb_data {

    ksnTRUDPClass *tu;
    uint32_t id;
    int fd;
    int cmd;
    int flags;
    PblMap *sl;
    __CONST_SOCKADDR_ARG addr;
    socklen_t addr_len;

} sl_timer_cb_data;

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


// Local methods
int ksnTRUDPSendListRemove(ksnTRUDPClass *tu, uint32_t id, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len);
int ksnTRUDPSendListAdd(ksnTRUDPClass *tu, uint32_t id, int fd, int cmd, 
        const void *data, size_t data_len, int flags, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len);
uint32_t ksnTRUDPSendListNewID(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr, 
        socklen_t addr_len);
void ksnTRUDPSendListDestroyAll(ksnTRUDPClass *tu);
PblMap *ksnTRUDPSendListGet(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr, 
        char *key_out);
size_t ksnTRUDPSendListGetKey( __CONST_SOCKADDR_ARG addr, char* key, 
        size_t key_len);
ip_map_data *ksnTRUDPIpMapData(ksnTRUDPClass *tu, 
        __CONST_SOCKADDR_ARG addr, char *key_out);
sl_data *ksnTRUDPSendListGetData(ksnTRUDPClass *tu, uint32_t id, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len);

ev_timer *sl_timer_start(ksnTRUDPClass *, PblMap *sl, uint32_t id, int fd, 
        int cmd, int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len);
void sl_timer_stop(EV_P_ ev_timer *w);
void sl_timer_cb(EV_P_ ev_timer *w, int revents);

int receive_compare(const void* prev, const void* next);
int ksnTRUDPReceiveHeapAdd(PblHeap *receive_heap, uint32_t id, void *data, 
        size_t data_len);
rh_data *ksnTRUDPReceiveHeapGetFirst(PblHeap *receive_heap);
int ksnTRUDPReceiveHeapElementFree(rh_data *rh_d);
int ksnTRUDPReceiveHeapRemoveFirst(PblHeap *receive_heap);
int ksnTRUDPReceiveHeapRemoveAt(PblHeap *receive_heap, int index);
void ksnTRUDPReceiveHeapRemoveAll(PblHeap *receive_heap);
void ksnTRUDPReceiveHeapDestroyAll(ksnTRUDPClass *tu);


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
        ksnTRUDPReceiveHeapDestroyAll(tu);
        
        pblMapFree(tu->ip_map);
        
        free(tu);
    }
}

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
ssize_t ksnTRUDPsendto(ksnTRUDPClass *tu, int fd, int cmd, const void *buf, 
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
ssize_t ksnTRUDPrecvfrom(ksnTRUDPClass *tu, int fd, void *buf, size_t buf_len, 
                         int flags, __SOCKADDR_ARG addr, socklen_t *addr_len) {
    
    const size_t tru_ptr = sizeof(ksnTRUDP_header); // Header size
                
    // Get data
    ssize_t recvlen = recvfrom(fd, buf, buf_len, flags, addr, addr_len);
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
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
            ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
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
                    
                    // TODO: If Message ID Equals to Expected ID send to core or 
                    // save to message Heap sorted by ID
                    // ksnTRUDPReceiveHeapAdd();    
                    
                    ip_map_data *ip_map_d = ksnTRUDPIpMapData(tu, addr, NULL);
                    
                    // Prepare to send to core if ID Equals to Expected ID
                    if(tru_header->id == ip_map_d->expected_id) {
                            
                        // Extract message from TR-UDP buffer
                        recvlen = tru_header->payload_length;
                        if(recvlen > 0) memmove(buf, buf + tru_ptr, recvlen);
                        
                        // Change Expected ID
                        ip_map_d->expected_id++;
                    } 
                    
                    // Save to Received message Heap
                    else {
                        
                        // TODO: Test the Heap to make sure it sorted right
                        ksnTRUDPReceiveHeapAdd(ip_map_d->receive_heap, 
                                tru_header->id, buf + tru_ptr, 
                                tru_header->payload_length); 
                        
                        // TODO: loop Received message Heap and send saved 
                        // messages recursively if records ID Equals to 
                        // Expected ID
                        //
                        // ip_map_d->expected_id
                    }
                                        
                }   break;
                
                // The ACK messages are used to acknowledge the arrival of the 
                // DATA and RESET messages. (has not payload)
                // Return zero length of this message.    
                case tru_ack: 
                {
                    
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                        "%sTR-UDP:%s +++ got ACK to message ID %d\n", 
                        ANSI_LIGHTGREEN, ANSI_NONE, 
                        tru_header->id
                    );
                    #endif
                   
                    // Get Send List timer watcher and stop it
                    sl_data *sl_d = ksnTRUDPSendListGetData(tu, tru_header->id, 
                            addr, *addr_len);
                    ev_timer *w = sl_d != NULL ? sl_d->w : NULL;
                    if(w != NULL) sl_timer_stop(kev->ev_loop, w);
                    
                    // Remove message from SendList
                    ksnTRUDPSendListRemove(tu, tru_header->id, addr, *addr_len);                  
                    
                    recvlen = 0;    // The received message is processed
                    
                }   break;
                    
                // The RESET messages reset messages counter. (has not payload)  
                // Return zero length of this message.    
                case tru_reset:
                    
                    #ifdef DEBUG_KSNET
                    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                        "%sTR-UDP:%s +++ got RESET command\n", 
                        ANSI_LIGHTGREEN, ANSI_NONE
                    );
                    #endif

                    // TODO: Process RESET command
                    recvlen = 0; // The received message is processed
                    break;
                    
                // Some error or undefined message. Don't process this message.
                // Return this message without any changes.    
                default:
                    break;
            }
        }
        else {
            
            #ifdef DEBUG_KSNET
            ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                "%sTR-UDP:%s skip received packet\n", 
                ANSI_LIGHTGREEN, ANSI_NONE
            );
            #endif
        }
    }
    
    return recvlen;
}

/**
 * Get IP map record by address or create new record if not exist
 * 
 * @param tu
 * @param addr
 * @param key_out [out]
 * 
 * @return 
 */
ip_map_data *ksnTRUDPIpMapData(ksnTRUDPClass *tu, 
        __CONST_SOCKADDR_ARG addr, char *key_out) {
    
    // Get ip map data by key
    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPSendListGetKey(addr, key, KSN_BUFFER_SM_SIZE);
    ip_map_data *ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);

    // Create new ip map record if it absent
    ip_map_data ip_map_d_new;
    if(ip_map_d == NULL) {
        
        ip_map_d_new.id = 0;
        ip_map_d_new.expected_id = 0;
        ip_map_d_new.send_list = pblMapNewHashMap();
        ip_map_d_new.receive_heap = pblHeapNew();
        pblHeapSetCompareFunction(ip_map_d_new.receive_heap, receive_compare);
        pblMapAdd(tu->ip_map, key, key_len, &ip_map_d_new, sizeof(ip_map_d_new));
        ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
          "%sTR-UDP:%s create new ip_map record with key %s (num records: %d)\n", 
          ANSI_LIGHTGREEN, ANSI_NONE,
          key, pblMapSize(tu->ip_map)
        );
        #endif  
    }
    
    // Copy key to output parameter
    if(key_out != NULL) strcpy(key_out, key);
    
    return ip_map_d;
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
size_t ksnTRUDPSendListGetKey( __CONST_SOCKADDR_ARG addr, char* key, 
        size_t key_len) {
    
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
int ksnTRUDPSendListRemove(ksnTRUDPClass *tu, uint32_t id, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {

    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPSendListGetKey(addr, key, KSN_BUFFER_SM_SIZE);
    ip_map_data *ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);
    if(ip_map_d != NULL) {
        
        pblMapRemove(ip_map_d->send_list, &id, sizeof(id), &val_len);
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
            "%sTR-UDP:%s message with id %d was removed from %s Send List "
            "(len %d)\n", 
            ANSI_LIGHTGREEN, ANSI_NONE,
            id, key, pblMapSize(ip_map_d->send_list)
        );
        #endif        
    }
    
    return 1;
}

/**
 * Get send list data
 * 
 * @param tu
 * @param id
 * @param addr
 * @param addr_len
 * @return 
 */
sl_data *ksnTRUDPSendListGetData(ksnTRUDPClass *tu, uint32_t id, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {

    sl_data *sl_d = NULL;
            
    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPSendListGetKey(addr, key, KSN_BUFFER_SM_SIZE);
    ip_map_data *ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);
    if(ip_map_d != NULL) {
        
        sl_d = pblMapGet(ip_map_d->send_list, &id, sizeof(id), &val_len);
    }
    
    return sl_d;
}  

/**
 * Get Send list by address or create new record if not exist
 * 
 * @param tu
 * @param addr
 * @param key_out
 * 
 * @return 
 */
inline PblMap *ksnTRUDPSendListGet(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr, 
        char *key_out) {

    return ksnTRUDPIpMapData(tu, addr, key_out)->send_list;
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
int ksnTRUDPSendListAdd(ksnTRUDPClass *tu, uint32_t id, int fd, int cmd, 
        const void *data, size_t data_len, int flags, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {
       
    // Get Send List by address from ip_map (or create new)
    char key[KSN_BUFFER_SM_SIZE];
    PblMap *sl = ksnTRUDPSendListGet(tu, addr, key);
    
    // Start ACK timeout timer watcher
    ev_timer *w = sl_timer_start(tu, sl, id, fd, cmd, flags, addr, addr_len);
    
    // Add message to Send List
    sl_data sl_d;
    sl_d.w = w;
    sl_d.data = (void*)data;
    sl_d.data_len = data_len;
    pblMapAdd(sl, &id, sizeof(id), (void*)&sl_d, sizeof(sl_d));
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
      "%sTR-UDP:%s message with id %d was added to %s Send List (len %d)\n", 
      ANSI_LIGHTGREEN, ANSI_NONE,
      id, key, pblMapSize(sl)
    );
    #endif
    
    return 1;
}

/**
 * Get new send message ID
 * 
 * @param tu
 * @param addr
 * @param addr_len
 * @return 
 */
inline uint32_t ksnTRUDPSendListNewID(ksnTRUDPClass *tu, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {
    
    return ksnTRUDPIpMapData(tu, addr, NULL)->id++;
}

/**
 * Destroy all Sent message lists
 * 
 * @param tu
 * @return 
 */
void ksnTRUDPSendListDestroyAll(ksnTRUDPClass *tu) {

    // TODO: Clear all Send List
    
}

/*******************************************************************************
 * 
 * Send list timer
 * 
 ******************************************************************************/

/**
 * Start list timer watcher
 * 
 * @param tu
 * @param sl
 * @param id
 * @param fd
 * @param cmd
 * @param flags
 * @param addr
 * @param addr_len
 * @return 
 */
ev_timer *sl_timer_start(ksnTRUDPClass *tu, PblMap *sl, uint32_t id, int fd, 
        int cmd, int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
      "%sTR-UDP:%s send list timer start, message id %d\n", 
      ANSI_LIGHTGREEN, ANSI_NONE,
      id
    );
    #endif

    ev_timer *w = malloc(sizeof(ev_timer));
    ev_timer_init(w, sl_timer_cb, MAX_ACK_WAIT, 0.0); ///< TODO: Set real timer value 
    sl_timer_cb_data *sl_t_data = malloc(sizeof(sl_timer_cb_data));
    sl_t_data->tu = tu;
    sl_t_data->sl = sl;
    sl_t_data->id = id;
    sl_t_data->fd = fd;
    sl_t_data->cmd = cmd;
    sl_t_data->flags = flags;
    sl_t_data->addr = addr;
    sl_t_data->addr_len = addr_len;
    w->data = sl_t_data;
    ev_timer_start(kev->ev_loop, w);
    
    return w;
}        

/**
 * Stop list timer watcher
 * 
 * @param loop
 * @param w
 * @return 
 */
inline void sl_timer_stop(EV_P_ ev_timer *w) {
    
    #ifdef DEBUG_KSNET
    ksnTRUDPClass *tu = ((sl_timer_cb_data *)w->data)->tu;
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
      "%sTR-UDP:%s send list timer stop, message id %d\n", 
      ANSI_LIGHTGREEN, ANSI_NONE,
      ((sl_timer_cb_data *)w->data)->id
    );
    #endif   

    // Stop timer
    ev_timer_stop(EV_A_ w);
    
    // Free watcher and its data
    free(w->data);
    free(w);
}
        
/**
 * Send list timer callback
 * 
 * @param loop
 * @param w
 * @param revents
 */
void sl_timer_cb(EV_P_ ev_timer *w, int revents) {
    
    sl_timer_cb_data *sl_t_data = w->data;
    ksnTRUDPClass *tu = sl_t_data->tu;
    
    // Get message from list
    size_t data_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPSendListGetKey(sl_t_data->addr, key, KSN_BUFFER_SM_SIZE);
    sl_data *sl_d = pblMapGet(sl_t_data->sl, key, key_len, &data_len);
    
    if(sl_d != NULL) {
        
        // Resend message
        ksnTRUDPsendto(tu, sl_t_data->fd, sl_t_data->cmd, sl_d->data, sl_d->data_len, 
                           sl_t_data->flags, sl_t_data->addr, sl_t_data->addr_len);

        // Remove record from list
        ksnTRUDPSendListRemove(tu, sl_t_data->id, sl_t_data->addr, sl_t_data->addr_len);
    
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
          "%sTR-UDP:%s timeout for message with id %d was happened, data resent\n", 
          ANSI_LIGHTGREEN, ANSI_NONE,
          sl_t_data->id
        );
        #endif

        sl_d->w = NULL;
    }
    
    else {
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
          "%sTR-UDP:%s timer for removed message with id %d was happened\n", 
          ANSI_LIGHTGREEN, ANSI_NONE,
          sl_t_data->id
        );
        #endif       
    }
   
    // Stop this timer
    sl_timer_stop(EV_A_ w);
}

/*******************************************************************************
 * 
 * Receive Heap
 * 
 ******************************************************************************/

/**
 * Receive Heap compare function
 * 
 * @param prev
 * @param next
 * @return 
 */
int receive_compare(const void* prev, const void* next) {
    
    int rv = 0;
    
    if(((rh_data*)prev)->id < ((rh_data*)next)->id) rv = -1;
    else if(((rh_data*)prev)->id > ((rh_data*)next)->id) rv = 1;
    
    return rv;
}

/**
 * Add record to the Receive Heap
 * 
 * @param receive_heap
 * @param id
 * @param data
 * @param data_len
 * @return 
 */
int ksnTRUDPReceiveHeapAdd(PblHeap *receive_heap, uint32_t id, void *data, 
        size_t data_len) {
    
    rh_data *rh_d = malloc(sizeof(rh_data));
    rh_d->id = id;
    rh_d->data = malloc(data_len);
    memcpy(rh_d->data, data, data_len);
        
    return pblHeapInsert(receive_heap, rh_d);;
}

/**
 * Get first element of Receive Heap (with lowest ID) 
 * 
 * @param receive_heap
 * @return  Pointer to rh_data or (void*)-1 at error - The heap is empty
 */
inline rh_data *ksnTRUDPReceiveHeapGetFirst(PblHeap *receive_heap) {
    
    return pblHeapGetFirst(receive_heap);
}

/**
 * Free Receive Heap data
 * 
 * @param rh_d
 * 
 * @return 1 if removed or 0 if element absent
 */
int ksnTRUDPReceiveHeapElementFree(rh_data *rh_d) {
    
    int rv = 0;
    if(rh_d != (void*)-1) {
        free(rh_d->data);
        free(rh_d);
        rv = 1;
    }
    
    return rv;
}

/**
 * Remove first element from Receive Heap (with lowest ID) 
 * 
 * @param receive_heap
 * 
 * @return 1 if element removed or 0 heap was empty
 */
inline int ksnTRUDPReceiveHeapRemoveFirst(PblHeap *receive_heap) {
    
    return ksnTRUDPReceiveHeapElementFree(pblHeapRemoveFirst(receive_heap));
}

/**
 * Remove element with selected index from Receive Heap
 * 
 * @param receive_heap
 * @param index
 * @return 
 */
inline int ksnTRUDPReceiveHeapRemoveAt(PblHeap *receive_heap, int index) {
    
    return ksnTRUDPReceiveHeapElementFree(pblHeapRemoveAt(receive_heap, index));
}

/**
 * Remove all elements from Receive Heap
 * 
 * @param receive_heap
 */
void ksnTRUDPReceiveHeapRemoveAll(PblHeap *receive_heap) {

    int i, num = pblHeapSize(receive_heap);
    for(i = num -1; i >= 0; i--) {
        ksnTRUDPReceiveHeapRemoveAt(receive_heap, i);
    }
}

/**
 * Destroy all Receive Heap
 * 
 * @param tu
 * @return 
 */
void ksnTRUDPReceiveHeapDestroyAll(ksnTRUDPClass *tu) {

    // TODO: Clear all Receive Heap
    
}
