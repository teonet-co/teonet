/** 
 * File:   tr-udp.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet Real time communications over UDP protocol (TR-UDP)
 *
 * Created on August 4, 2015, 12:16 AM
 */

#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "tr-udp.h"
#include "tr-udp_stat.h"
#include "config/conf.h"
#include "utils/utils.h"
#include "utils/rlutil.h"

// Teonet TCP proxy functions
ssize_t teo_recvfrom (ksnetEvMgrClass* ke, 
            int fd, void *buffer, size_t buffer_len, int flags,
            __SOCKADDR_ARG addr, socklen_t *__restrict addr_len);
ssize_t teo_sendto (ksnetEvMgrClass* ke,
            int fd, const void *buffer, size_t buffer_len, int flags, 
            __CONST_SOCKADDR_ARG addr, socklen_t addr_len);

//#define VERSION_MAJOR 0
//#define VERSION_MINOR 0

#define kev ((ksnetEvMgrClass*)(((ksnCoreClass*)tu->kc)->ke))

#include "tr-udp_.h"


/*****************************************************************************
 *
 *  Main functions
 * 
 *****************************************************************************/

/**
 * Initialize TR-UDP class
 * 
 * @param kc Pointer to ksnCoreClass object 
 * @return Pointer to created ksnTRUDPClass object
 */
ksnTRUDPClass *ksnTRUDPinit(void *kc) {

    ksnTRUDPClass *tu = malloc(sizeof (ksnTRUDPClass));
    tu->kc = kc;
    tu->started = 0; 
    tu->ip_map = pblMapNewHashMap();
    ksnTRUDPregisterProcessPacket(tu, ksnCoreProcessPacket);
    ksnTRUDPstatInit(tu);
    
    return tu;
}

/**
 * Destroy TR-UDP class
 * 
 * @param tu Pointer to ksnTRUDPClass object
 */
void ksnTRUDPDestroy(ksnTRUDPClass *tu) {

    if (tu != NULL) {

        ksnTRUDPsendListDestroyAll(tu); // Destroy all send lists
        ksnTRUDPreceiveHeapDestroyAll(tu); // Destroy all receive heap       
        pblMapFree(tu->ip_map); // Free IP map

        free(tu); // Free class data
    }
}

/**
 * Remove all records in IP map (include all send lists and receive heaps)
 * 
 * @param tu Pointer to ksnTRUDPClass object
 */
void ksnTRUDPremoveAll(ksnTRUDPClass *tu) {

    if (tu != NULL) {

        ksnTRUDPsendListDestroyAll(tu); // Destroy all send lists
        ksnTRUDPreceiveHeapDestroyAll(tu); // Destroy all receive heap       
        pblMapClear(tu->ip_map); // Clear IP map
    }
}

// Make TR-UDP Header
#define MakeHeader(tru_header, packet_id, type, buf_len) \
    tru_header.version = TR_UDP_PROTOCOL_VERSION; \
    tru_header.payload_length = buf_len; \
    tru_header.message_type = type; \
    tru_header.id = packet_id; \
    tru_header.timestamp = ksnTRUDPtimestamp(); \
    SetHeaderChecksum(tru_header)

// Calculate and set checksum in TR-UDP Header
#define SetHeaderChecksum(tru_header) \
    ksnTRUDPchecksumSet(&tru_header, ksnTRUDPchecksumCalculate(&tru_header))
                
/**
 * Send to peer through TR-UDP transport
 * 
 * @param tu Pointer to ksnTRUDPClass object
 * @param resend_flg New message or resend sent before (0 - new, 1 -resend)
 * @param id ID of resend message
 * @param cmd Command to allow TR-UDP
 * @param attempt Number of attempt of this message
 * @param fd File descriptor of UDP connection
 * @param buf Buffer with data
 * @param buf_len Data length
 * @param flags Flags (always 0, reserved)
 * @param addr Peer address
 * @param addr_len Peer address length
 * 
 * @return Number of bytes sent to UDP
 */
ssize_t ksnTRUDPsendto(ksnTRUDPClass *tu, int resend_flg, uint32_t id, 
        int attempt, int cmd, int fd, const void *buf, size_t buf_len, int flags, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s got %d bytes data to send, cmd %d\n",
            ANSI_LIGHTGREEN, ANSI_NONE,
            buf_len, cmd
    );
    #endif

    // TR-UDP: Check commands array
    if(CMD_TRUDP_CHECK(cmd)) {

        // TR-UDP packet buffer
        const size_t tru_ptr = sizeof (ksnTRUDP_header); // Header size
        char tru_buf[KSN_BUFFER_DB_SIZE]; // Packet buffer
        ksnTRUDP_header tru_header; // Header buffer

        //! \todo: If payload_length more than buffer crop data or drop this packet
        if (buf_len + tru_ptr > KSN_BUFFER_DB_SIZE) {
            buf_len = KSN_BUFFER_DB_SIZE - tru_ptr;
        }

        // Make TR-UDP Header
        MakeHeader(tru_header, 
                   resend_flg ? id : ksnTRUDPsendListNewID(tu, addr), 
                   TRU_DATA, 
                   buf_len);
        
        // Copy TR-UDP header
        memcpy(tru_buf, &tru_header, tru_ptr);

        // Copy TR-UDP message
        memcpy(tru_buf + tru_ptr, buf, buf_len);
        
        // Add (or update) record to send list
        ksnTRUDPsendListAdd(tu, tru_header.id, fd, cmd, buf, buf_len, flags, 
                attempt, addr, addr_len);        

        // New buffer length
        buf_len += tru_ptr;
        buf = tru_buf;

        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                "%sTR-UDP:%s >> send %d bytes message of type: %d "
                "with %d bytes data payload to %s:%d\n",
                ANSI_LIGHTGREEN, ANSI_NONE,
                buf_len, tru_header.message_type,
                tru_header.payload_length,
                inet_ntoa(((struct sockaddr_in *) addr)->sin_addr),
                ntohs(((struct sockaddr_in *) addr)->sin_port)
        );
        #endif

        // Set packets time statistics
        ksnTRUDPsetDATAsendTime(tu, addr);                    
        
        // Set statistic send list size
        if(!resend_flg) ksnTRUDPstatSendListAdd(tu);           
    } 
    
    // Not TR-UDP
    else {
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                "%sTR-UDP:%s >> skip this packet, "
                "send %d bytes direct by UDP to: %s:%d\n",
                ANSI_LIGHTGREEN, ANSI_NONE,
                buf_len,
                inet_ntoa(((struct sockaddr_in *) addr)->sin_addr),
                ntohs(((struct sockaddr_in *) addr)->sin_port)
        );
        #endif
    }

    return buf_len > 0 ? teo_sendto(kev, fd, buf, buf_len, flags, addr, addr_len) : 
                         buf_len;
}

/**
 * Get data from peer through TR-UDP transport
 * 
 * @param tu
 * @param fd
 * @param buffer
 * @param buffer_len
 * @param flags
 * @param addr
 * @param addr_len
 * 
 * @return If return 0 than the packet is processed by tu->process_packet 
 *         function. In other case there is value returned by UDP recvfrom 
 *         function and the buffer contain received data
 */
ssize_t ksnTRUDPrecvfrom(ksnTRUDPClass *tu, int fd, void *buffer, 
                         size_t buffer_len, int flags, __SOCKADDR_ARG addr, 
                         socklen_t *addr_len) {
    
    /*
     * Send ACK to sender subroutine  
     * 
     */
    #define ksnTRUDPSendACK() { \
        \
        ksnTRUDP_header tru_send_header; \
        memcpy(&tru_send_header, tru_header, tru_ptr); \
        tru_send_header.payload_length = 0; \
        tru_send_header.message_type = TRU_ACK; \
        tru_send_header.checksum = ksnTRUDPchecksumCalculate(&tru_send_header); \
        const socklen_t addr_len = sizeof(struct sockaddr_in); \
        teo_sendto(kev, fd, &tru_send_header, tru_ptr, 0, addr, addr_len); \
    }    

    const size_t tru_ptr = sizeof (ksnTRUDP_header); // Header size

    // Get data 
    ssize_t recvlen = teo_recvfrom(kev, fd, buffer, buffer_len, flags, addr, addr_len);    

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s << got %d bytes packet, from %s:%d\n",
            ANSI_LIGHTGREEN, ANSI_NONE,
            (int)recvlen, inet_ntoa(((struct sockaddr_in *) addr)->sin_addr),
            ntohs(((struct sockaddr_in *) addr)->sin_port)
    );
    #endif

    // Data received
    if (recvlen > 0) {

        ksnTRUDP_header *tru_header = buffer;
        
        // Check for TR-UDP header and its checksum
        if (/*arp != NULL &&*/
            recvlen - tru_ptr == tru_header->payload_length && 
            ksnTRUDPchecksumCheck(tru_header)) {
            
            // Check for new peer
            ip_map_data *ipm;
            ksnet_arp_data *arp = NULL;
            if((ipm = ksnTRUDPipMapDataTry(tu, addr, NULL, 0)) != NULL) {
                if((arp = ipm->arp) == NULL)
                    arp = ksnetArpFindByAddr(kev->kc->ka, addr);
            }
            if(arp == NULL) { // Ignore TR-UDP request if peer not connected yet 
                
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                    "%sTR-UDP:%s Ignore message: The remote peer has not registered in this host yet\n",
                    ANSI_LIGHTGREEN, ANSI_NONE                    
                );
                #endif

            } else  { 
            
                #ifdef DEBUG_KSNET
                ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                    "%sTR-UDP:%s process %d bytes message of type %d, id %d, "
                    "with %d bytes data payload\n",
                    ANSI_LIGHTGREEN, ANSI_NONE,
                    (int)recvlen, tru_header->message_type, tru_header->id, 
                    tru_header->payload_length
                );
                #endif

                // Set last activity time
                ksnTRUDPsetActivity(tu, addr);                    

                switch (tru_header->message_type) {

                    // The DATA messages are carrying payload. (has payload)
                    // Process this message.
                    // Return message with payload if there is expected message or 
                    // zero len message if this message was push to queue.
                    case TRU_DATA:
                    {
                        // Send ACK to sender
                        ksnTRUDPSendACK();

                        // If Message ID Equals to Expected ID send message to core
                        // or save to message Heap sorted by ID
                        // ksnTRUDPReceiveHeapAdd();    

                        // Calculate times statistic
                        ksnTRUDPsetDATAreceiveTime(tu, addr);                    

                        // Read IP Map
                        ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, addr, NULL, 0);

                        // Send to core if ID Equals to Expected ID
                        if (tru_header->id == ip_map_d->expected_id) {

                            // Change Expected ID
                            ip_map_d->expected_id++;

                            #ifdef DEBUG_KSNET
                            ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                                "%sTR-UDP:%s recvfrom: Processed id %d from %s:%d\n", 
                                ANSI_LIGHTGREEN, ANSI_NONE,    
                                tru_header->id,
                                inet_ntoa(((struct sockaddr_in *) addr)->sin_addr),
                                ntohs(((struct sockaddr_in *) addr)->sin_port)
                            );
                            #endif

                            // Process packet
                            tu->process_packet(kev->kc, buffer + tru_ptr, 
                                    tru_header->payload_length, addr);

                            // Check Received message Heap and send saved 
                            // messages to core if first records ID Equals to 
                            // Expected ID
                            int num;
                            while ((num = pblHeapSize(ip_map_d->receive_heap))) {

                                rh_data *rh_d = ksnTRUDPreceiveHeapGetFirst(
                                        ip_map_d->receive_heap);

                                #ifdef DEBUG_KSNET
                                ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                                        "%sTR-UDP:%s recvfrom: Check Receive Heap,"
                                        " len = %d, id = %d ... ",
                                        ANSI_LIGHTGREEN, ANSI_NONE,
                                        num, rh_d->id
                                );
                                #endif

                                // Process this message
                                if (rh_d->id == ip_map_d->expected_id) {

                                    #ifdef DEBUG_KSNET
                                    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                                        "Processed\n");
                                    #endif

                                    // Process packet
                                    tu->process_packet(kev->kc, rh_d->data, 
                                            rh_d->data_len, &rh_d->addr);

                                    // Remove first record
                                    ksnTRUDPreceiveHeapRemoveFirst(tu, 
                                            ip_map_d->receive_heap);

                                    // Change Expected ID
                                    ip_map_d->expected_id++;
                                }

                                // Remove already processed
                                else if (rh_d->id < ip_map_d->expected_id) {

                                    #ifdef DEBUG_KSNET
                                    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                                        "Removed\n");
                                    #endif

                                    // Remove first record
                                    ksnTRUDPreceiveHeapRemoveFirst(tu, 
                                            ip_map_d->receive_heap);
                                }

                                // Drop saved message
                                else {

                                    #ifdef DEBUG_KSNET
                                    ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                                        "Skipped\n");
                                    #endif

                                    break; // while
                                }
                            }
                        }

                        // Drop old (repeated) message 
                        else if (tru_header->id < ip_map_d->expected_id) {

                            #ifdef DEBUG_KSNET
                            ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                                "%sTR-UDP:%s recvfrom: Drop old (repeated) message "
                                "with id %d\n",
                                ANSI_LIGHTGREEN, ANSI_NONE,
                                tru_header->id);
                            #endif

                            ksnTRUDPsetDATAreceiveDropped(tu, addr);
                        }   

                        // Save to Received message Heap
                        else {

                            #ifdef DEBUG_KSNET
                            ksnet_printf(&kev->ksn_cfg, DEBUG_VV, 
                                "%sTR-UDP:%s recvfrom: Add to receive heap, "
                                "id = %d, len = %d, expected id = %d\n",
                                ANSI_LIGHTGREEN, ANSI_NONE,
                                tru_header->id, tru_header->payload_length,
                                ip_map_d->expected_id);
                            #endif

                            ksnTRUDPreceiveHeapAdd(tu, ip_map_d->receive_heap,
                                    tru_header->id, buffer + tru_ptr,
                                    tru_header->payload_length, addr, *addr_len);
                        }
                    }

                    recvlen = 0; // The received message is processed
                    break;

                    // The ACK messages are used to acknowledge the arrival of the 
                    // DATA and RESET messages. (has not payload)
                    // Return zero length of this message.    
                    case TRU_ACK:
                    {

                        #ifdef DEBUG_KSNET
                        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                                "%sTR-UDP:%s +++ got ACK to message ID %d\n",
                                ANSI_LIGHTGREEN, ANSI_NONE,
                                tru_header->id
                        );
                        #endif

                        // Calculate times statistic
                        ksnTRUDPsetACKtime(tu, addr, tru_header);

                        // Send event to application
                        if(kev->event_cb != NULL) {

                            sl_data *sl_d = ksnTRUDPsendListGetData(tu, tru_header->id, addr);
                            if(sl_d != NULL) {

                                char *data = sl_d->data_buf; 
                                size_t data_len = sl_d->data_len; 
                                #if KSNET_CRYPT
                                if(kev->ksn_cfg.crypt_f && ksnCheckEncrypted(data, data_len)) {
                                    data = ksnDecryptPackage(kev->kc->kcr, data, data_len, &data_len);
                                }
                                #endif
                                ksnCorePacketData rd;
                                memset(&rd, 0, sizeof(rd));

                                // Remote peer address and peer
                                rd.addr = inet_ntoa(((struct sockaddr_in*)addr)->sin_addr); // IP to string
                                rd.port = ntohs(((struct sockaddr_in*)addr)->sin_port); // Port to integer

                                // Parse packet and check if it valid
                                if(ksnCoreParsePacket(data, data_len, &rd)) {

                                    // Send event for CMD for Application level TR-UDP mode: 128...191
                                    if(rd.cmd >= 128 && rd.cmd < 192) {

                                        kev->event_cb(kev, EV_K_RECEIVED_ACK, 
                                            (void*)&rd, // Pointer to ksnCorePacketData
                                            sizeof(rd), // Length of ksnCorePacketData
                                            &tru_header->id); // Pointer to packet ID
                                    }
                                }
                            }
                        }

                        // Remove message from SendList and stop timer watcher
                        ksnTRUDPsendListRemove(tu, tru_header->id, addr);   
                    }
                    recvlen = 0; // The received message is processed
                    break;

                    // The RESET messages reset messages counter. (has not payload)  
                    // Return zero length of this message.    
                    case TRU_RESET:

                        #ifdef DEBUG_KSNET
                        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                                "%sTR-UDP:%s +++ got RESET command\n",
                                ANSI_LIGHTGREEN, ANSI_NONE
                        );
                        #endif

                        // Process RESET command
                        ksnTRUDPreset(tu, addr, 0);
                        ksnTRUDPstatReset(tu); //! \todo: Do we need reset it here?
                        ksnTRUDPSendACK(); // Send ACK

                        recvlen = 0; // The received message is processed
                        break;

                        // Some error or undefined message. Don't process this message.
                        // Return this message without any changes.    
                    default:
                        break;
                }
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
 * Register process packet function for the TR-UDP recvfrom function
 * 
 * @param tu
 * @param pc
 */
inline void* ksnTRUDPregisterProcessPacket(ksnTRUDPClass *tu, 
        ksnTRUDPprocessPacketCb pc) {
    
    ksnTRUDPprocessPacketCb process_packet = tu->process_packet;
    tu->process_packet = pc;    
    return process_packet;
}

/*****************************************************************************
 *
 *  Header checksum functions
 * 
 *****************************************************************************/

/**
 * Calculate TR-UDP header checksum
 * 
 * @param th
 * @return 
 */
uint8_t ksnTRUDPchecksumCalculate(ksnTRUDP_header *th) {
    
    int i;
    uint8_t checksum = 0;
    for(i = 1; i < sizeof(ksnTRUDP_header); i++) {
        checksum += *((uint8_t*) th + i);
    }
    
    return checksum;
}

/**
 * Set TR-UDP header checksum
 * 
 * @param th
 * @param chk
 * @return 
 */
inline void ksnTRUDPchecksumSet(ksnTRUDP_header *th, uint8_t chk) {
    
    th->checksum = chk;
}

/**
 * Check TR-UDP header checksum
 * 
 * @param th
 * @return 
 */
inline int ksnTRUDPchecksumCheck(ksnTRUDP_header *th) {
      
    return th->checksum == ksnTRUDPchecksumCalculate(th);
}

/*****************************************************************************
 *
 *  Utility functions
 * 
 *****************************************************************************/

/**
 * Get IP map record by address or create new record if not exist
 * 
 * @param tu Pointer ksnTRUDPClass
 * @param addr Peer address (created in sockaddr_in structure)
 * @param key_out [out] Buffer to copy created from addr key. Don't copy if NULL
 * @param key_out_len Length of buffer to copy created key
 * 
 * @return Pointer to ip_map_data or NULL if not found
 */
ip_map_data *ksnTRUDPipMapData(ksnTRUDPClass *tu,
        __CONST_SOCKADDR_ARG addr, char *key_out, size_t key_out_len) {

    // Get ip map data by key
    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPkeyCreate(tu, addr, key, KSN_BUFFER_SM_SIZE);
    ip_map_data *ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);

    // Create new ip map record if it absent
    if (ip_map_d == NULL) {

        ip_map_data ip_map_d_new;
        
        ip_map_d_new.id = 0;
        ip_map_d_new.expected_id = 0;
        ip_map_d_new.send_list = pblMapNewHashMap();
        ip_map_d_new.receive_heap = pblHeapNew();
        ip_map_d_new.arp = ksnetArpFindByAddr(kev->kc->ka, addr);
        pblHeapSetCompareFunction(ip_map_d_new.receive_heap,
                ksnTRUDPreceiveHeapCompare);
        pblMapAdd(tu->ip_map, key, key_len, &ip_map_d_new, sizeof (ip_map_d_new));
        ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);
        ksnTRUDPstatAddrInit(tu, addr);

        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                "%sTR-UDP:%s create new ip_map record with key %s "
                "(num records: %d)\n",
                ANSI_LIGHTGREEN, ANSI_NONE,
                key, pblMapSize(tu->ip_map)
        );        
        #endif  
    }

    // Copy key to output parameter
    if (key_out != NULL) strncpy(key_out, key, key_out_len);

    return ip_map_d;
}

/**
 * Get IP map record by address
 * 
 * @param tu Pointer ksnTRUDPClass
 * @param addr Peer address (created in sockaddr_in structure)
 * @param key_out [out] Buffer to copy created from addr key. Don't copy if NULL
 * @param key_out_len Length of buffer to copy created key
 * 
 * @return Pointer to ip_map_data or NULL if not found
 */
ip_map_data *ksnTRUDPipMapDataTry(ksnTRUDPClass *tu,
        __CONST_SOCKADDR_ARG addr, char *key_out, size_t key_out_len) {

    // Get ip map data by key
    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPkeyCreate(0, addr, key, KSN_BUFFER_SM_SIZE);
    ip_map_data *ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);

    // Copy key to output parameter
    if (key_out != NULL) strncpy(key_out, key, key_out_len);

    return ip_map_d;
}

/**
 * Create key from address
 * 
 * @param tu Pointer ksnTRUDPClass
 * @param addr Peer address (created in sockaddr_in structure)
 * @param key [out] Buffer to copy created from addr key. Don't copy if NULL
 * @param key_len Length of buffer to copy created key
 * 
 * @return Created key length
 */
size_t ksnTRUDPkeyCreate(ksnTRUDPClass* tu, __CONST_SOCKADDR_ARG addr,
        char* key, size_t key_len) {

    return snprintf(key, key_len, "%s:%d",
            inet_ntoa(((struct sockaddr_in *) addr)->sin_addr),
            ntohs(((struct sockaddr_in *) addr)->sin_port));
}

/**
 * Create key from address and port
 * 
 * @param tu Pointer ksnTRUDPClass
 * @param addr String with address (ip)
 * @param port Port number
 * @param key [out] Buffer to copy created from addr key. Don't copy if NULL
 * @param key_len Length of buffer to copy created key
 * 
 * @return Created key length 
 */
inline size_t ksnTRUDPkeyCreateAddr(ksnTRUDPClass* tu, const char *addr, int port, char* key,
        size_t key_len) {

    return snprintf(key, key_len, "%s:%d", addr, port);
}

#ifdef MINGW
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64 

// MSVC defines this in winsock2.h!?
typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}
#endif

#include <sys/time.h>

/**
 * Get current 32 bit timestamp in thousands of milliseconds
 * 
 * @return 
 */
uint32_t ksnTRUDPtimestamp() {
    
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    //long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    long long milliseconds = te.tv_sec*1000000LL + te.tv_usec; // calculate thousands of milliseconds
    return (uint32_t) (milliseconds & 0xFFFFFFFF);
}

// Make address from string
int make_addr(const char *addr, int port, __SOCKADDR_ARG remaddr, socklen_t *addr_len) {
    
    if(*addr_len < sizeof(struct sockaddr_in)) return -3;
    *addr_len = sizeof(struct sockaddr_in); // length of addresses
    memset((char *) remaddr, 0, *addr_len);
    ((struct sockaddr_in*)remaddr)->sin_family = AF_INET;
    ((struct sockaddr_in*)remaddr)->sin_port = htons(port);
    #ifndef HAVE_MINGW
    if(inet_aton(addr, &((struct sockaddr_in*)remaddr)->sin_addr) == 0) {
        return(-2);
    }
    #else
    ((struct sockaddr_in*)remaddr)->sin_addr.s_addr = inet_addr(addr);
    #endif

    return 0;
}

/**
 * Set last activity time
 * 
 * @param tu
 * @param addr
 * @return 
 */
void ksnTRUDPsetActivity(ksnTRUDPClass* tu, __CONST_SOCKADDR_ARG addr) {
    
    ip_map_data *ip_map_d = ksnTRUDPipMapDataTry(tu, addr, NULL, 0);
    
    if(ip_map_d != NULL) {
        if(ip_map_d->arp != NULL) 
            ip_map_d->arp->last_activity = ksnetEvMgrGetTime(kev);
    }
}

/*****************************************************************************
 *
 *  Reset functions
 * 
 *****************************************************************************/

/**
 * Remove send list and receive heap by input address
 * 
 * @param tu Pointer to ksnTRUDPClass
 * @param addr Peer address
 * @param options Reset options:
 *          0 - reset mode:  clear send list and receive heap 
 *          1 - remove mode: clear send list and receive heap, 
 *                           and remove record from IP Map
 */
void ksnTRUDPreset(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr, int options) {

    // Create key from address
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPkeyCreate(0, addr, key, KSN_BUFFER_SM_SIZE);

    // Reset by key
    ksnTRUDPresetKey(tu, key, key_len, options);
}

/**
 * Remove send list and receive heap by address and port
 * 
 * @param tu Pointer to ksnTRUDPClass
 * @param addr String with IP address
 * @param port IP port number
 * @param options Reset options:
 *          0 - reset mode:  clear send list and receive heap 
 *          1 - remove mode: clear send list and receive heap, 
 *                           and remove record from IP Map
 */
void ksnTRUDPresetAddr(ksnTRUDPClass *tu, const char *addr, int port, int options) {

    // Create key from address
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPkeyCreateAddr(0, addr, port, key, KSN_BUFFER_SM_SIZE);

    // Reset by key
    ksnTRUDPresetKey(tu, key, key_len, options);
}

/**
 * TR-UDP Reset by key
 * 
 * Remove send list and receive heap by key.
 * 
 * @param tu Pointer to ksnTRUDPClass
 * @param key Key string in format %s:%d - ip:port
 * @param key_len Length of key string
 * @param options Reset options:
 *          0 - reset mode:  clear send list and receive heap 
 *          1 - remove mode: clear send list and receive heap, 
 *                           and remove record from IP Map
 */
void ksnTRUDPresetKey(ksnTRUDPClass *tu, char *key, size_t key_len, int options) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s reset %s, options: %d\n",
            ANSI_LIGHTGREEN, ANSI_NONE,
            key, options
    );
    #endif    

    // Get ip_map 
    size_t val_len;
    ip_map_data *ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);

    // Remove send list and receive heap
    if (ip_map_d != NULL) {

        // Reset or remove Send List
        ksnTRUDPsendListRemoveAll(tu, ip_map_d->send_list); // Remove all elements from Send List
        if (options) {
            pblMapFree(ip_map_d->send_list); // Free Send List
            ip_map_d->send_list = NULL; // Clear Send List pointer
        }
        ip_map_d->id = 0; // Reset send message ID

        // Reset or remove Receive Heap
        ksnTRUDPreceiveHeapRemoveAll(tu, ip_map_d->receive_heap); // Remove all elements from Receive Heap
        if (options) {
            pblHeapFree(ip_map_d->receive_heap); // Free Receive Heap   
            ip_map_d->receive_heap = NULL; // Clear Receive Heap pointer
        }
        ip_map_d->expected_id = 0; // Reset receive heap expected ID

        // Reset statistic
        _ksnTRUDPstatAddrInit(ip_map_d);
        
        // Remove IP map record (in remove mode))
        if (options) {
            pblMapRemove(tu->ip_map, key, key_len, &val_len);
        }                
    }
}

/**
 * Send reset command to peer and add it to send list
 * 
 * @param tu
 * @param fd
 * @param addr
 */
void ksnTRUDPresetSend(ksnTRUDPClass *tu, int fd, __CONST_SOCKADDR_ARG addr) {

    // Send reset command to peer
    ksnTRUDP_header tru_header; // Header buffer
    MakeHeader(tru_header, ksnTRUDPsendListNewID(tu, addr), TRU_RESET, 0); // Make TR-UDP Header
    const socklen_t addr_len = sizeof(struct sockaddr_in); // Length of addresses   
    teo_sendto(kev, fd, &tru_header, sizeof(tru_header), 0, addr, addr_len); // Send
    
    //! \todo: Add sent reset to send list
}


/*****************************************************************************
 *
 *  Send list functions
 * 
 *****************************************************************************/

/**
 * Remove message from Send message List (Acknowledge Pending Messages)
 * 
 * @param tu
 * @param id
 * @param addr
 * @return 1 - removed; 0 - record does not exist
 * 
 */
int ksnTRUDPsendListRemove(ksnTRUDPClass *tu, uint32_t id,
        __CONST_SOCKADDR_ARG addr) {

    int retval = 0;
    
    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPkeyCreate(0, addr, key, KSN_BUFFER_SM_SIZE);
    ip_map_data *ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);
    if (ip_map_d != NULL) {

        // Stop send list timer and remove from map
        sl_data *sl_d = pblMapGet(ip_map_d->send_list, &id, sizeof (id), &val_len);
        if(sl_d != NULL) {
            
            // Stop send list timer
            sl_timer_stop(kev->ev_loop, &sl_d->w);

            // Remove record from send list
            pblMapRemove(ip_map_d->send_list, &id, sizeof (id), &val_len);
            
            // Statistic
            ksnTRUDPstatSendListRemove(tu);    

            retval = 1;
        }
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                "%sTR-UDP:%s message with id %d was removed from %s Send List "
                "(len %d)\n",
                ANSI_LIGHTGREEN, ANSI_NONE,
                id, key, pblMapSize(ip_map_d->send_list)
        );
        #endif        
    }

    return retval;
}

/**
 * Get send list data
 * 
 * @param tu
 * @param id
 * @param addr
 * @return 
 */
sl_data *ksnTRUDPsendListGetData(ksnTRUDPClass *tu, uint32_t id,
        __CONST_SOCKADDR_ARG addr) {

    sl_data *sl_d = NULL;

    size_t val_len;
    char key[KSN_BUFFER_SM_SIZE];
    size_t key_len = ksnTRUDPkeyCreate(0, addr, key, KSN_BUFFER_SM_SIZE);
    ip_map_data *ip_map_d = pblMapGet(tu->ip_map, key, key_len, &val_len);
    if (ip_map_d != NULL) {

        sl_d = pblMapGet(ip_map_d->send_list, &id, sizeof (id), &val_len);
    }

    return sl_d;
}

/**
 * Get Send list by address or create new record if not exist
 * 
 * @param tu
 * @param addr
 * @param key_out
 * @param key_out_len
 * 
 * @return 
 */
inline PblMap *ksnTRUDPsendListGet(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr,
        char *key_out, size_t key_out_len) {

    return ksnTRUDPipMapData(tu, addr, key_out, key_out_len)->send_list;
}

/**
 * Add packet to Sent message list
 * 
 * Add packet to Sent message list (Acknowledge Pending Messages) and start 
 * timer to wait ACK
 * 
 * @param tu
 * @param id
 * @param fd
 * @param cmd
 * @param data
 * @param data_len Length of data, should be less or equal than KSN_BUFFER_SIZE
 * @param flags
 * @param attempt
 * @param addr
 * @param addr_len
 * 
 * @return 
 */
int ksnTRUDPsendListAdd(ksnTRUDPClass *tu, uint32_t id, int fd, int cmd,
        const void *data, size_t data_len, int flags, int attempt, 
        __CONST_SOCKADDR_ARG addr, socklen_t addr_len) {

    // Get Send List by address from ip_map (or create new)
    char key[KSN_BUFFER_SM_SIZE];
    PblMap *sl = ksnTRUDPsendListGet(tu, addr, key, KSN_BUFFER_SM_SIZE);

    // Add message to Send List
    const size_t sl_d_len = sizeof(sl_data) + data_len;
    char sl_d_buf[sl_d_len];
    sl_data *sl_d = (void*) sl_d_buf;
    sl_d->attempt = attempt;
    sl_d->data_len = data_len; // < KSN_BUFFER_SIZE ? data_len : KSN_BUFFER_SIZE;
    memcpy(sl_d->data_buf, data, sl_d->data_len);
    pblMapAdd(sl, &id, sizeof (id), (void*) sl_d, sl_d_len);

    // Start ACK timer watcher
    size_t valueLength;
    sl_data *sl_d_get = pblMapGet(sl, &id, sizeof (id), &valueLength);
    sl_timer_start(&sl_d_get->w, &sl_d_get->w_data, tu, id, fd, cmd, flags, 
            addr, addr_len); 
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s message with id %d, command %d was added "
            "to %s Send List (len %d)\n",
            ANSI_LIGHTGREEN, ANSI_NONE,
            id, cmd, key, pblMapSize(sl)
    );
    #endif

    return 1;
}

/**
 * Get new send message ID
 * 
 * @param tu
 * @param addr
 * @return 
 */
inline uint32_t ksnTRUDPsendListNewID(ksnTRUDPClass *tu,
        __CONST_SOCKADDR_ARG addr) {

    return ksnTRUDPipMapData(tu, addr, NULL, 0)->id++;
}

/**
 * Remove all elements from Send List
 * 
 * @param tu
 * @param send_list
 */
void ksnTRUDPsendListRemoveAll(ksnTRUDPClass *tu, PblMap *send_list) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s sent message lists remove all\n",
            ANSI_LIGHTGREEN, ANSI_NONE
    );
    #endif

    PblIterator *it = pblMapIteratorReverseNew(send_list);
    if (it != NULL) {
        while (pblIteratorHasPrevious(it)) {
            void *entry = pblIteratorPrevious(it);
            sl_data *sl_d = pblMapEntryValue(entry);
            sl_timer_stop(kev->ev_loop, &sl_d->w);
            ksnTRUDPstatSendListRemove(tu);
        }
        pblIteratorFree(it);
    }
    pblMapClear(send_list);
}

/**
 * Free all elements and free all Sent message lists
 * 
 * @param tu
 * @return 
 */
void ksnTRUDPsendListDestroyAll(ksnTRUDPClass *tu) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s sent message lists destroy all\n",
            ANSI_LIGHTGREEN, ANSI_NONE
    );
    #endif

    PblIterator *it = pblMapIteratorReverseNew(tu->ip_map);
    if (it != NULL) {
        while (pblIteratorHasPrevious(it)) {
            void *entry = pblIteratorPrevious(it);
            ip_map_data *ip_map_d = pblMapEntryValue(entry);
            if(ip_map_d->send_list != NULL) {
                ksnTRUDPsendListRemoveAll(tu, ip_map_d->send_list);
                pblMapFree(ip_map_d->send_list);
                ip_map_d->send_list = NULL;
            }
        }
        pblIteratorFree(it);
    }
}


/*******************************************************************************
 * 
 * Send list timer functions
 * 
 ******************************************************************************/

/**
 * Start list timer watcher
 * 
 * @param w
 * @param w_data
 * @param tu
 * @param id
 * @param fd
 * @param cmd
 * @param flags
 * @param addr
 * @param addr_len
 * @return 
 */
ev_timer *sl_timer_start(ev_timer *w, void *w_data, ksnTRUDPClass *tu, 
        uint32_t id, int fd, int cmd, int flags, __CONST_SOCKADDR_ARG addr, 
        socklen_t addr_len) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s send list timer start, message id %d\n",
            ANSI_LIGHTGREEN, ANSI_NONE,
            id
    );
    #endif

    // Timer value 
    ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, addr, NULL, 0);
    double max_ack_wait = ip_map_d->stat.triptime_last_max / 1000000.0;
    if(max_ack_wait > 0) {
        max_ack_wait += max_ack_wait * 
            (ip_map_d->stat.packets_attempt < 10 ? 0.5 : 0.75);
    }
    else max_ack_wait = MAX_ACK_WAIT; // Default value
    
    // Initialize, set user data and start the timer
    ev_timer_init(w, sl_timer_cb, max_ack_wait, 0.0);
    sl_timer_cb_data *sl_t_data = w_data;
    sl_t_data->tu = tu;
    sl_t_data->id = id;
    sl_t_data->fd = fd;
    sl_t_data->cmd = cmd;
    sl_t_data->flags = flags;
    memcpy(&sl_t_data->addr, addr, addr_len);
    sl_t_data->addr_len = addr_len;
    w->data = sl_t_data;        
    ev_timer_start(kev->ev_loop, w);
   
    return w;
}

/**
 * Stop list timer watcher
 * 
 * param loop
 * @param w
 * @return 
 */
inline void sl_timer_stop(EV_P_ ev_timer *w) {

    if(w->data != NULL) {
        
        #ifdef DEBUG_KSNET
        ksnTRUDPClass *tu = ((sl_timer_cb_data *) w->data)->tu;
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                "%sTR-UDP:%s send list timer stop, message id %d\n",
                ANSI_LIGHTGREEN, ANSI_NONE,
                ((sl_timer_cb_data *) w->data)->id
        );
        #endif   

        // Stop timer
        ev_timer_stop(EV_A_ w);
        w->data = NULL;
    }
}

/**
 * Send list timer callback
 * 
 * The timer event appears if timer was not stopped during timeout. I means that 
 * packet was not received by peer and we need resend this packet.
 * 
 * param loop Event loop 
 * @param w Watcher
 * @param revents Events (not used, reserved)
 */
void sl_timer_cb(EV_P_ ev_timer *w, int revents) {

    sl_timer_cb_data sl_t_data;
    memcpy(&sl_t_data, w->data, sizeof(sl_timer_cb_data));
    ksnTRUDPClass *tu = sl_t_data.tu;
    
    // Stop this timer
    sl_timer_stop(EV_A_ w);    

    // Get message from list
    sl_data *sl_d = ksnTRUDPsendListGetData(tu, sl_t_data.id, &sl_t_data.addr);

    if (sl_d != NULL) {

        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                "%sTR-UDP:%s %stimeout for message with id %d was happened%s, "
                "resend %d bytes data to %s:%d\n",
                ANSI_LIGHTGREEN, ANSI_NONE,
                ANSI_RED, 
                sl_t_data.id,
                ANSI_NONE, 
                (int) sl_d->data_len,
                inet_ntoa(((struct sockaddr_in *) &sl_t_data.addr)->sin_addr),
                ntohs(((struct sockaddr_in *) &sl_t_data.addr)->sin_port)
                
        );
        #endif
        
        // Resend message
        ksnTRUDPsendto(tu, 1, sl_t_data.id, sl_d->attempt+1, sl_t_data.cmd, 
                sl_t_data.fd, sl_d->data_buf, sl_d->data_len, sl_t_data.flags, 
                &sl_t_data.addr, sl_t_data.addr_len);
        
        // Statistic
        ksnTRUDPstatSendListAttempt(tu, &sl_t_data.addr);    

    } else {
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s timer for removed message with id %d was happened\n",
            ANSI_LIGHTGREEN, ANSI_NONE, sl_t_data.id
        );
        #endif       
    }
}

/*******************************************************************************
 * 
 * Receive Heap functions
 * 
 ******************************************************************************/

/**
 * Receive Heap compare function. Set lower ID first
 * 
 * @param prev
 * @param next
 * @return 
 */
int ksnTRUDPreceiveHeapCompare(const void* prev, const void* next) {

    int rv = 0;

    if ((*((rh_data**) prev))->id > (*((rh_data**) next))->id) rv = -1;
    else
    if ((*((rh_data**) prev))->id < (*((rh_data**) next))->id) rv = 1;

    return rv;
}

/**
 * Add record to the Receive Heap
 * 
 * @param tu
 * @param receive_heap
 * @param id
 * @param data
 * @param data_len
 * @param addr
 * @param addr_len
 * 
 * @return 
 */
int ksnTRUDPreceiveHeapAdd(ksnTRUDPClass *tu, PblHeap *receive_heap, 
        uint32_t id, void *data, size_t data_len, __CONST_SOCKADDR_ARG addr, 
        socklen_t addr_len) {

    #ifdef DEBUG_KSNET
    if (tu != NULL) {
        char key[KSN_BUFFER_SM_SIZE];
        ksnTRUDPkeyCreate(0, addr, key, KSN_BUFFER_SM_SIZE);
        ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
                "%sTR-UDP:%s receive heap %s add %d bytes\n",
                ANSI_LIGHTGREEN, ANSI_NONE,
                key, data_len
        );
    }
    #endif

    // Create receive heap data
    rh_data *rh_d = malloc(sizeof(rh_data) + data_len);
    rh_d->id = id;
    rh_d->data_len = data_len;
    memcpy(rh_d->data, data, data_len);
    memcpy(&rh_d->addr, addr, addr_len);

    // Statistic
    ksnTRUDPstatReceiveHeapAdd(tu);

    return pblHeapInsert(receive_heap, rh_d);
}

/**
 * Get first element of Receive Heap (with lowest ID) 
 * 
 * @param receive_heap
 * @return  Pointer to rh_data or (void*)-1 at error - The heap is empty
 */
inline rh_data *ksnTRUDPreceiveHeapGetFirst(PblHeap *receive_heap) {

    return pblHeapGetFirst(receive_heap);
}

/**
 * Free Receive Heap data
 * 
 * @param rh_d
 * 
 * @return 1 if removed or 0 if element is absent
 */
int ksnTRUDPreceiveHeapElementFree(rh_data *rh_d) {

    int rv = 0;
    if(rh_d != (void*) - 1) {
        free(rh_d);
        rv = 1;
    }

    return rv;
}

/**
 * Remove first element from Receive Heap (with lowest ID) 
 * 
 * @param tu
 * @param receive_heap
 * 
 * @return 1 if element removed or 0 heap was empty
 */
inline int ksnTRUDPreceiveHeapRemoveFirst(ksnTRUDPClass *tu, PblHeap *receive_heap) {

    // Statistic
    ksnTRUDPstatReceiveHeapRemove(tu);

    return ksnTRUDPreceiveHeapElementFree(pblHeapRemoveFirst(receive_heap));
}

/**
 * Remove all elements from Receive Heap
 * 
 * @param tu
 * @param receive_heap
 */
void ksnTRUDPreceiveHeapRemoveAll(ksnTRUDPClass *tu, PblHeap *receive_heap) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s receive heap remove all\n",
            ANSI_LIGHTGREEN, ANSI_NONE
    );
    #endif
    
    if(receive_heap != NULL) {
        int i, num = pblHeapSize(receive_heap);
        for (i = num - 1; i >= 0; i--) {
            ksnTRUDPreceiveHeapElementFree(pblHeapRemoveAt(receive_heap, i));
            ksnTRUDPstatReceiveHeapRemove(tu);
        }
        pblHeapClear(receive_heap);
    }
}

/**
 * Free all elements and free all Receive Heap
 * 
 * @param tu
 * @return 
 */
void ksnTRUDPreceiveHeapDestroyAll(ksnTRUDPClass *tu) {

    #ifdef DEBUG_KSNET
    ksnet_printf(&kev->ksn_cfg, DEBUG_VV,
            "%sTR-UDP:%s receive heap destroy all\n",
            ANSI_LIGHTGREEN, ANSI_NONE
    );
    #endif

    PblIterator *it = pblMapIteratorReverseNew(tu->ip_map);
    if (it != NULL) {
        while (pblIteratorHasPrevious(it)) {
            void *entry = pblIteratorPrevious(it);
            ip_map_data *ip_map_d = pblMapEntryValue(entry);
            if(ip_map_d->receive_heap != NULL) {
                ksnTRUDPreceiveHeapRemoveAll(tu, ip_map_d->receive_heap);
                pblHeapFree(ip_map_d->receive_heap);
                ip_map_d->receive_heap = NULL;
            }
        }
        pblIteratorFree(it);
    }
}

//! \todo: Expand source code documentation

//! \todo:  Describe Reset and Reset with remove in WiKi
