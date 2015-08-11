/** 
 * File:   net_tr-udp_stat.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * TR-UDP statistic
 *
 * Created on August 11, 2015, 2:34 PM
 */

#include "net_tr-udp.h"
#include "net_tr-udp_stat.h"
#include "utils/utils.h"

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
 * @return 
 */
inline size_t ksnTRUDPstatSendListAttempt(ksnTRUDPClass *tu) {

    if(tu == NULL) return 0;
    
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
 * @return 
 */
inline char * ksnTRUDPstatShow(ksnTRUDPClass *tu) {

    return ksnet_formatMessage(
        "----------------------------------------\n"
        "RT-UDP statistics:\n"
        "----------------------------------------\n"
        "Send list:\n"
        "  size_max: %d\n"
        "  size_current: %d\n"
        "  attempts: %d\n"
        "Receive Heap:\n"
        "  size_max: %d\n"
        "  size_current: %d\n"
        "----------------------------------------\n"
        , tu->stat.send_list.size_max
        , tu->stat.send_list.size_current
        , tu->stat.send_list.attempt
        , tu->stat.receive_heap.size_max
        , tu->stat.receive_heap.size_current
    );
}
