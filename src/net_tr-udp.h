/** 
 * File:   net_tr-udp.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet Real time communications over UDP protocol (TR-UDP)
 *
 * Created on August 4, 2015, 12:16 AM
 */

#ifndef NET_TR_UDP_H
#define	NET_TR_UDP_H

/**
 * Teonet TR-UDP class data
 */
typedef struct ksnTrUdpClass {
    
    void *kc; ///< Pointer to KSNet core class object
    
} ksnTrUdpClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnTrUdpClass *ksnTrUdpInit(void *kc);
void ksnTrUdpDestroy(ksnTrUdpClass *tu);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_TR_UDP_H */

