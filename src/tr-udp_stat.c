/** 
 * File:   tr-udp_stat.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * TR-UDP statistic
 *
 * Created on August 11, 2015, 2:34 PM
 */
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "tr-udp_.h"
#include "tr-udp_stat.h"
#include "utils/utils.h"

#include "net_core.h"

#define MODULE _ANSI_LIGHTGREEN "tr_udp_stat" _ANSI_NONE

/******************************************************************************
 * 
 * TR-UDP Common statistic
 * 
 ******************************************************************************/ 

/**
 * Initialize TR-UDP statistic submodule
 * 
 * @param tu
 * @return 
 */
tr_udp_stat *ksnTRUDPstatInit(ksnTRUDPClass *tu) {
    
    if(tu == NULL) return 0;

    tu->stat.receive_heap.size_current = 0;
    tu->stat.receive_heap.size_max = 0;
    
    tu->stat.send_list.size_current = 0;
    tu->stat.send_list.size_max = 0;
    tu->stat.send_list.attempt = 0;
    
    return &tu->stat;
}

inline void ksnTRUDPstatReset(ksnTRUDPClass *tu) {
    
    ksnTRUDPstatInit(tu);
}

/**
 * Increment send list size
 * 
 * @param tu
 * @return 
 */
inline size_t ksnTRUDPstatSendListAdd(ksnTRUDPClass *tu) {
    
    if(tu == NULL) return 0;

    tu->stat.send_list.size_current++;
    if(tu->stat.send_list.size_current > tu->stat.send_list.size_max) 
        tu->stat.send_list.size_max = tu->stat.send_list.size_current;
    
    return tu->stat.send_list.size_current;
}

/**
 * Decrement send list size
 * 
 * @param tu
 * @return 
 */
inline size_t ksnTRUDPstatSendListRemove(ksnTRUDPClass *tu) {
    
    if(tu == NULL) return 0;

    return --tu->stat.send_list.size_current;
}

/**
 * Increment number of send list attempts
 * 
 * @param tu
 * @param addr
 * @return 
 */
inline size_t ksnTRUDPstatSendListAttempt(ksnTRUDPClass *tu, 
        __CONST_SOCKADDR_ARG addr) {

    if(tu == NULL) return 0;
    
    ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, addr, NULL, 0);
    ip_map_d->stat.packets_attempt++;
            
    return ++tu->stat.send_list.attempt;
}    
        
/**
 * Increment receive heap size
 * 
 * @param tu
 * @return 
 */
inline size_t ksnTRUDPstatReceiveHeapAdd(ksnTRUDPClass *tu) {
    
    if(tu == NULL) return 0;
        
    tu->stat.receive_heap.size_current++;
    if(tu->stat.receive_heap.size_current > tu->stat.receive_heap.size_max)
        tu->stat.receive_heap.size_max = tu->stat.receive_heap.size_current;
    
    return tu->stat.receive_heap.size_current;
}

/**
 * Increment receive heap size
 * 
 * @param tu
 * @return 
 */
inline size_t ksnTRUDPstatReceiveHeapRemove(ksnTRUDPClass *tu) {

    if(tu == NULL) return 0;
    
    return --tu->stat.receive_heap.size_current;
}        


/**
 * Show TR-UDP statistics
 * 
 * Return string with statistics. It should be free after use.
 * 
 * @param tu
 * 
 * @return Pointer to allocated string with statistics
 */
inline char * ksnTRUDPstatShowStr(ksnTRUDPClass *tu) {

    
    uint32_t packets_send = 0, packets_receive = 0, ack_receive = 0, 
             packets_dropped = 0;
     
    int i = 0;
    char *tbl_str = strdup(null_str);
    PblIterator *it =  pblMapIteratorNew(tu->ip_map);
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {
        
            void *entry = pblIteratorNext(it);
            char *key = pblMapEntryKey(entry);
            size_t key_len = pblMapEntryKeyLength(entry);
            ip_map_data *ip_map_d = pblMapEntryValue(entry);
            packets_send += ip_map_d->stat.packets_send;
            ack_receive += ip_map_d->stat.ack_receive;
            packets_receive += ip_map_d->stat.packets_receive;
            packets_dropped += ip_map_d->stat.packets_receive_dropped;
            
            tbl_str = ksnet_sformatMessage(tbl_str,  
                "%s%3d "_ANSI_BROWN"%-20.*s"_ANSI_NONE" %8d %11.3f %10.3f %9.3f %8d %8d %8d %8d %6d %6d\n", 
                tbl_str, i + 1, 
                key_len, key, 
                ip_map_d->stat.packets_send, 
                (double)(1.0 * ip_map_d->stat.send_speed / 1024.0),    
                ip_map_d->stat.send_total,    
                ip_map_d->stat.wait, 
                ip_map_d->stat.packets_receive, 
                ip_map_d->stat.ack_receive, 
                ip_map_d->stat.packets_receive_dropped,
                ip_map_d->stat.packets_attempt,
                pblMapSize(ip_map_d->send_list), 
                pblHeapSize(ip_map_d->receive_heap)
            );
            i++;
        }
        pblIteratorFree(it);
    }
    
    char *ret_str = ksnet_formatMessage(
        _ANSI_CLS"\033[0;0H"
        "\n"
        "----------------------------------------------------------------------------------------------------------------------\n"
        "TR-UDP statistics:\n"
        "----------------------------------------------------------------------------------------------------------------------\n"
        "Run time: %f sec\n"
        "\n"
        "Packets sent: %d\n"
        "ACK receive: %d\n"
        "Packets receive: %d\n"
        "Packets receive and dropped: %d\n"
        "\n"
        "Send list:\n"
        "  size_max: %d\n"
        "  size_current: %d\n"
        "  attempts: %d\n"
        "\n"
        "Receive Heap:\n"
        "  size_max: %d\n"
        "  size_current: %d\n"
        "\n"
        "----------------------------------------------------------------------------------------------------------------------\n"
        "  # Key                      Send  Speed(kb/s)  Total(mb)  Wait(ms)    Recv      ACK   Repeat     Drop     SQ     RQ  \n"
        "----------------------------------------------------------------------------------------------------------------------\n"
        "%s"
        "----------------------------------------------------------------------------------------------------------------------\n"
        "  "
        _ANSI_GREEN"send:"_ANSI_NONE" send packets, "
        _ANSI_GREEN"speed:"_ANSI_NONE" send speed(kb/s), "
        _ANSI_GREEN"total:"_ANSI_NONE" send in megabytes, "
        _ANSI_GREEN"wait:"_ANSI_NONE" time to wait ACK, "
        _ANSI_GREEN"recv:"_ANSI_NONE" receive packets   \n"
        
        "  "
        _ANSI_GREEN"ACK:"_ANSI_NONE" receive ACK,   "
        _ANSI_GREEN"repeat:"_ANSI_NONE" resend packets,  "
        _ANSI_GREEN"drop:"_ANSI_NONE" receive duplicate,  "
        _ANSI_GREEN"SQ:"_ANSI_NONE" send queue,         "
        _ANSI_GREEN"RQ:"_ANSI_NONE" receive queue       \n"
        , ksnetEvMgrGetTime(((ksnCoreClass *)tu->kc)->ke) - tu->started
        , packets_send
        , ack_receive
        , packets_receive
        , packets_dropped
        
        , tu->stat.send_list.size_max
        , tu->stat.send_list.size_current
        , tu->stat.send_list.attempt
        , tu->stat.receive_heap.size_max
        , tu->stat.receive_heap.size_current
        , tbl_str
    );
    
    free(tbl_str);
    
    return ret_str;
}

/**
 * Show TR-UDP statistics on terminal
 * 
 * Print string with statistics on terminal. 
 * 
 * @param tu
 * 
 * @return Number if line in statistics text
 */
inline int ksnTRUDPstatShow(ksnTRUDPClass *tu) {

    int num_line = 0;
    char *str = ksnTRUDPstatShowStr(tu);

    ksn_printf(((ksnetEvMgrClass*)(((ksnCoreClass*)tu->kc)->ke)), MODULE, 
            DISPLAY_M, 
            "%s", str);
    
    num_line = calculate_lines(str);

    free(str);

    return num_line;
}

/******************************************************************************
 * 
 * TR-UDP statistic by address
 * 
 ******************************************************************************/ 

void _ksnTRUDPstatAddrInit(ip_map_data *ip_map_d) {
    
    memset(&ip_map_d->stat, 0, sizeof(ip_map_d->stat));
    ip_map_d->stat.triptime_min = UINT32_MAX;
}

/**
 * Initialize packets time statistics by address
 * 
 * @param tu
 * @param addr
 */
void ksnTRUDPstatAddrInit(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr) {
    
    ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, addr, NULL, 0);    
    _ksnTRUDPstatAddrInit(ip_map_d);
}

inline void ksnTRUDPstatAddrReset(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr) {
    
    ksnTRUDPstatAddrInit(tu, addr);
}

void ksnTRUDPstatAddrResetAll(ksnTRUDPClass *tu) {
    
    PblIterator *it = pblMapIteratorReverseNew(tu->ip_map);
    if (it != NULL) {
        while (pblIteratorHasPrevious(it) > 0) {
            void *entry = pblIteratorPrevious(it);
            ip_map_data *ip_map_d = pblMapEntryValue(entry);
            _ksnTRUDPstatAddrInit(ip_map_d);
        }
        pblIteratorFree(it);
    }
}

/**
 * Destroy packets time statistics by address
 * 
 * @param tu
 * @param addr
 */
void ksnTRUDPstatAddrDestroy(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr) {
    
    //ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, addr, NULL, 0);
}

/**
 * Save packets time statistics by address when ACK received
 * 
 * @param tu
 * @param addr
 * @param tru_header
 */
void ksnTRUDPsetACKtime(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr, 
        ksnTRUDP_header *tru_header) {

    int i;
    ip_map_data *ip_map_d = ksnTRUDPipMapDataTry(tu, addr, NULL, 0);
    
    if(ip_map_d != NULL) {
        
        // Calculate triptime last, minimal and maximum
        ip_map_d->stat.triptime_last = ksnTRUDPtimestamp() - tru_header->timestamp;
        ip_map_d->stat.triptime_avg = 
                ((uint64_t)ip_map_d->stat.ack_receive * 
                    ip_map_d->stat.triptime_avg + ip_map_d->stat.triptime_last) / 
                (ip_map_d->stat.ack_receive + 1); 
        ip_map_d->stat.ack_receive++;
        if(ip_map_d->stat.triptime_last < ip_map_d->stat.triptime_min) 
            ip_map_d->stat.triptime_min = ip_map_d->stat.triptime_last;
        if(ip_map_d->stat.triptime_last > ip_map_d->stat.triptime_max) 
            ip_map_d->stat.triptime_max = ip_map_d->stat.triptime_last;

        // Add triptime to last 10 array
        ip_map_d->stat.last_send_packets_ar[ip_map_d->stat.idx_snd].triptime = ip_map_d->stat.triptime_last;
        
        // Add last data size to last 10 array
        size_t val_len;
        sl_data *sl_d = pblMapGet(ip_map_d->send_list, &tru_header->id, sizeof(tru_header->id), &val_len);
        if(sl_d != NULL) {
            ip_map_d->stat.last_send_packets_ar[ip_map_d->stat.idx_snd].size_b = sl_d->data_len + sizeof(ksnTRUDP_header);
            ip_map_d->stat.send_total += 1.0 * (sl_d->data_len + sizeof(ksnTRUDP_header)) / (1024.0 * 1024.0);
            ip_map_d->stat.last_send_packets_ar[ip_map_d->stat.idx_snd].ts = tru_header->timestamp;
        }
        else {
            ip_map_d->stat.last_send_packets_ar[ip_map_d->stat.idx_snd].size_b = 0;
            ip_map_d->stat.last_send_packets_ar[ip_map_d->stat.idx_snd].ts = 0;
        }
                
        // Last 10 array next index
        ip_map_d->stat.idx_snd++;
        if(ip_map_d->stat.idx_snd >= LAST10_SIZE) ip_map_d->stat.idx_snd = 0;
        

        // Calculate max triptime & speed in bytes in sec in last 10 packet
        {
            uint32_t min_ts = UINT32_MAX, max_ts = 0, size_b = 0;
            uint32_t triptime_last_max = 0;
            for(i = 0; i < LAST10_SIZE; i++) {
                
                // Last maximum triptime
                if(ip_map_d->stat.last_send_packets_ar[i].triptime > triptime_last_max)
                    triptime_last_max = ip_map_d->stat.last_send_packets_ar[i].triptime;
                
                // Calculate size sum & define minimum and maximum timestamp
                size_b += ip_map_d->stat.last_send_packets_ar[i].size_b;
                if(ip_map_d->stat.last_send_packets_ar[i].ts > max_ts) max_ts = ip_map_d->stat.last_send_packets_ar[i].ts;
                else if (ip_map_d->stat.last_send_packets_ar[i].ts > 0 && ip_map_d->stat.last_send_packets_ar[i].ts < min_ts) min_ts = ip_map_d->stat.last_send_packets_ar[i].ts;
            }
            
            // Last maximal triptime
            ip_map_d->stat.triptime_last_max = (ip_map_d->stat.triptime_last_max + 2 * triptime_last_max) / 3;
            
            // Send speed
            uint32_t dif_ts = max_ts - min_ts;
            if(dif_ts) ip_map_d->stat.send_speed = 1.0 * size_b / (1.0 * (dif_ts) / 1000000.0);
            else ip_map_d->stat.send_speed = 0;
        }
    }
}

/**
 * Save packets time statistics by address when DATE send
 * 
 * @param tu
 * @param addr
 */
inline void ksnTRUDPsetDATAsendTime(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr) {
    
    ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, addr, NULL, 0);   
    ip_map_d->stat.packets_send++;
}

/**
 * Save packets time statistics by address when DATE receive
 * 
 * @param tu
 * @param addr
 */
inline void ksnTRUDPsetDATAreceiveTime(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr) {
    
    ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, addr, NULL, 0);   
    ip_map_d->stat.packets_receive++;
}

inline void ksnTRUDPsetDATAreceiveDropped(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr) {
    
    ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, addr, NULL, 0);   
    ip_map_d->stat.packets_receive_dropped++;
}
