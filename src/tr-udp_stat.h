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

#include "tr-udp.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define MODULE _ANSI_LIGHTGREEN "tr_udp_stat" _ANSI_NONE

#include "trudp_stat.h"
#include "ev_mgr.h"

#define KE(tu) (ksnetEvMgrClass *)tu->user_data

int ksnTRUDPstatShow(trudpData *tu);
int ksnTRUDPqueuesShow(trudpData *td);


#ifdef	__cplusplus
}
#endif

#endif	/* NET_TR_UDP_STAT_H */
