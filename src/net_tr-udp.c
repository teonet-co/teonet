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

/**
 * Initialize TR-UDP class
 * 
 * @param kc
 * @return 
 */
ksnTRUDPClass *ksnTRUDPInit(void *kc) {
    
    ksnTRUDPClass *tu = malloc(sizeof(ksnTRUDPClass));
    tu->kc = kc;
            
    return tu;
}

/**
 * Destroy TR-UDP class
 * 
 * @param tu
 */
void ksnTRUDPDestroy(ksnTRUDPClass *tu) {
    
    if(tu != NULL) {
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
    static uint32_t msg_id = 0;

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
        tru_header.id = msg_id++;   

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
    } 
    else {
        #ifdef DEBUG_KSNET
        ksnet_printf(&ke->ksn_cfg, DEBUG_VV, 
            "%sTR-UDP:%s >> skip this packet, send %d bytes by UDP to: %s:%d\n", 
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
                    
                    // TODO: If Message ID Equals to Expected ID send to core or 
                    // save to message Heap sorted by ID
                    
                    // Extract message from TR-UDP buffer
                    recvlen = tru_header->payload_length;
                    if(recvlen > 0) memmove(buf, buf + tru_ptr, recvlen);
                    
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
