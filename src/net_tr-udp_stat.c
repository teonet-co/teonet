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

tr_udp_stat *ksnTRUDPstatInit(ksnTRUDPClass *tu) {
    
    tu->stat.receive_heap.size_current = 0;
    tu->stat.receive_heap.size_max = 0;
    
    tu->stat.send_list.size_current = 0;
    tu->stat.send_list.size_max = 0;
    tu->stat.send_list.attempt = 0;
    
    return &tu->stat;
}
