/** 
 * File:   net_tr-udp_.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Private module structures and function definition
 *
 * Created on August 7, 2015, 11:34 PM
 */


#ifndef NET_TR_UDP__H
#define	NET_TR_UDP__H

#define MAX_ACK_WAIT 0.100  // 100 MS

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
 * Receive heap data
 */
typedef struct rh_data {
    
    uint32_t id;
    void *data;
    size_t data_len;
    struct sockaddr addr;
    socklen_t addr_len;
    
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
    __CONST_SOCKADDR_ARG addr;
    socklen_t addr_len;

} sl_timer_cb_data;

/**
 * Send list data structure
 */
typedef struct sl_data {
    
    ev_timer w;
    sl_timer_cb_data w_data;
    void *data;
    size_t data_len;
    size_t attempt;

} sl_data;

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
    TRU_DATA, ///< The DATA messages are carrying payload. (has payload)
    /**
     * The ACK messages are used to acknowledge the arrival of the DATA and 
     * RESET messages. (has not payload) 
     */
    TRU_ASK,
    TRU_RESET ///< The RESET messages reset messages counter. (has not payload)

};


// Local methods
size_t ksnTRUDPkeyCreate(ksnTRUDPClass* tu, __CONST_SOCKADDR_ARG addr, char* key,
        size_t key_len);
size_t ksnTRUDPkeyCreateAddr(ksnTRUDPClass* tu, const char *addr, int port, 
        char* key, size_t key_len);
ip_map_data *ksnTRUDPipMapData(ksnTRUDPClass *tu,
        __CONST_SOCKADDR_ARG addr, char *key_out, size_t key_len);
//
int ksnTRUDPsendListRemove(ksnTRUDPClass *tu, uint32_t id,
        __CONST_SOCKADDR_ARG addr);
int ksnTRUDPsendListAdd(ksnTRUDPClass *tu, uint32_t id, int fd, int cmd,
        const void *data, size_t data_len, int flags, int attempt,
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len);
uint32_t ksnTRUDPsendListNewID(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr);
void ksnTRUDPsendListDestroyAll(ksnTRUDPClass *tu);
PblMap *ksnTRUDPsendListGet(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr,
        char *key_out, size_t key_len);
sl_data *ksnTRUDPsendListGetData(ksnTRUDPClass *tu, uint32_t id,
        __CONST_SOCKADDR_ARG addr);
void ksnTRUDPsendListRemoveAll(ksnTRUDPClass *tu, PblMap *send_list);
//
ev_timer *sl_timer_start(ev_timer *w, void *w_data, ksnTRUDPClass *, uint32_t id, int fd,
        int cmd, int flags, __CONST_SOCKADDR_ARG addr, socklen_t addr_len);
void sl_timer_stop(EV_P_ ev_timer *w);
void sl_timer_cb(EV_P_ ev_timer *w, int revents);
//
int ksnTRUDPreceiveHeapCompare(const void* prev, const void* next);
int ksnTRUDPreceiveHeapAdd(ksnTRUDPClass *tu, PblHeap *receive_heap,
        uint32_t id, void *data,
        size_t data_len, __SOCKADDR_ARG addr, socklen_t addr_len);
rh_data *ksnTRUDPreceiveHeapGetFirst(PblHeap *receive_heap);
int ksnTRUDPreceiveHeapElementFree(rh_data *rh_d);
int ksnTRUDPreceiveHeapRemoveFirst(PblHeap *receive_heap);
void ksnTRUDPreceiveHeapRemoveAll(ksnTRUDPClass *tu, PblHeap *receive_heap);
void ksnTRUDPreceiveHeapDestroyAll(ksnTRUDPClass *tu);
//
void ksnTRUDPreset(ksnTRUDPClass *tu, __SOCKADDR_ARG addr, int options);
void ksnTRUDPresetAddr(ksnTRUDPClass *tu, const char *addr, int port, int options);
void ksnTRUDPresetKey(ksnTRUDPClass *tu, char *key, size_t key_len, int options);
void ksnTRUDPresetSend(ksnTRUDPClass *tu, int fd, __SOCKADDR_ARG addr);


#ifdef	__cplusplus
}
#endif

#endif	/* NET_TR_UDP__H */

