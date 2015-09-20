/** 
 * File:   tr-udp_stat.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * TR-UDP statistic
 *
 * Created on August 11, 2015, 2:34 PM
 */

#ifndef NET_TR_UDP_STAT_H
#define	NET_TR_UDP_STAT_H

#include "tr-udp_.h"

#ifdef	__cplusplus
extern "C" {
#endif

tr_udp_stat *ksnTRUDPstatInit(ksnTRUDPClass *tu);
void ksnTRUDPstatReset(ksnTRUDPClass *tu);
void ksnTRUDPstatAddrResetAll(ksnTRUDPClass *tu);
size_t ksnTRUDPstatSendListAdd(ksnTRUDPClass *tu);
size_t ksnTRUDPstatSendListRemove(ksnTRUDPClass *tu);
size_t ksnTRUDPstatSendListAttempt(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr);
size_t ksnTRUDPstatReceiveHeapAdd(ksnTRUDPClass *tu);
size_t ksnTRUDPstatReceiveHeapRemove(ksnTRUDPClass *tu);
char * ksnTRUDPstatShowStr(ksnTRUDPClass *tu);
int ksnTRUDPstatShow(ksnTRUDPClass *tu);

void _ksnTRUDPstatAddrInit(ip_map_data *ip_map_d);
void ksnTRUDPstatAddrInit(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr);
void ksnTRUDPstatAddrDestroy(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr);
void ksnTRUDPstatAddrReset(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr);
void ksnTRUDPsetACKtime(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr, 
        ksnTRUDP_header *tru_header);
void ksnTRUDPsetDATAsendTime(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr);
void ksnTRUDPsetDATAreceiveTime(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr);
void ksnTRUDPsetDATAreceiveDropped(ksnTRUDPClass *tu, __CONST_SOCKADDR_ARG addr);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_TR_UDP_STAT_H */
