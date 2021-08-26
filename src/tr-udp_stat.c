/**
 * File:   tr-udp_stat.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * TR-UDP statistic
 *
 * Created on August 11, 2015, 2:34 PM
 */

#include "tr-udp.h"
#include "tr-udp_stat.h"
#include <stdlib.h>

/**
 * Show TR-UDP statistics on terminal
 *
 * Print string with statistics on terminal.
 *
 * @param tu
 *
 * @return Number if line in statistics text
 */
inline int ksnTRUDPstatShow(trudpData *tu) {

    int num_line = 0;
    char *str = ksnTRUDPstatShowStr(tu, 0);
    
    cls();
    printf("%s", str);

    num_line = calculate_lines(str);

    free(str);

    return num_line;
}

/**
 * Show TR-UDP queues list on terminal
 *
 * Print string with queues list on terminal.
 *
 * @param td Pointer to trudpData
 *
 * @return Number if line in statistics text
 */
inline int ksnTRUDPqueuesShow(trudpData *td) {

    int num_line = 0;
    int max_q_size = -1;
    trudpChannelData *tcd = (void*)-1;
    
    // Get first queues element
//    trudpChannelData *tcd = (trudpChannelData*)trudpMapGetFirst(td->map, 0);
    
    // Loop all channel maps
    teoMapElementData *el;
    teoMapIterator *it;
    if((it = teoMapIteratorNew(td->map))) {
        while((el = teoMapIteratorNext(it))) {
            
            trudpChannelData *_tcd = (trudpChannelData *)
                    teoMapIteratorElementData(el, NULL);
            
            // Get element with max queue size
            int sqs = trudpSendQueueSize(_tcd->sendQueue), rqs = trudpReceiveQueueSize(_tcd->receiveQueue);
            if(sqs > max_q_size) { max_q_size = sqs; tcd = _tcd; }
            if(rqs > max_q_size) { max_q_size = rqs; tcd = _tcd; }
        }
        teoMapIteratorFree(it);
    }
    
    if(tcd != (void*)-1) {

        char *stat_sq_str = trudpStatShowQueueStr(tcd, 0);
        if(stat_sq_str) {
            cls();
            
            int port; //,type;
            //uint32_t id = trudpPacketGetId(data);
            char *addr = trudpUdpGetAddr((__CONST_SOCKADDR_ARG)&tcd->remaddr, tcd->addrlen, &port);
            printf("--------------------------------------------------------------\n" 
                   "TR-UDP channel "_ANSI_BROWN"%s:%d:%d"_ANSI_NONE" queues:\n\n", 
                   addr, port, tcd->channel);
            puts(stat_sq_str);
            free(stat_sq_str);
        }
        else cls();

        char *stat_rq_str = trudpStatShowQueueStr(tcd, 1);
        if(stat_rq_str) {
            puts(stat_rq_str);
            free(stat_rq_str);
        }
    }
    else { cls(); puts("Queues have not been created..."); }

    return num_line;
}
