/** 
 * File:   subscribe.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on December 28, 2015, 1:31 PM
 */

#ifndef SUBSCRIBE_H
#define	SUBSCRIBE_H

#include <stdint.h>
#include <pbl.h>

#include "teonet_l0_client.h"

/**
 * teoSScr Class structure definition
 */
typedef struct teoSScrClass {
    
    void *ke; ///< Pointer to ksnetEvMgrClass
    PblMap *map; ///< Pointer to the subscribers map
    
} teoSScrClass;

/**
 * teoSScr class list or CMD_SUBSCRIBE_ANSWER data
 */
typedef struct teoSScrListData {
    
    uint16_t ev; ///< Event (used when send data to subscriber)
    uint8_t cmd; ///< Command ID (used when send data to subscriber)
    uint8_t l0_f; ///< This is L0 client. The L0 server name added to the beginning of data
    char addr[ARP_TABLE_IP_SIZE]; ///< L0 peer IP address
    int16_t port; ///< L0 peer port    
    char data[]; ///< Remote peer name in list or data in CMD_SUBSCRIBE_ANSWER
        
} teoSScrListData;

#ifdef	__cplusplus
extern "C" {
#endif

teoSScrClass *teoSScrInit(void *ke);
void teoSScrDestroy(teoSScrClass *sscr);
void teoSScrSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev, ksnet_arp_data *arp);
int teoSScrUnSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev);
int teoSScrUnSubscriptionAll(teoSScrClass *sscr, char *peer_name);
const char *teoSScrSubscriptionList(teoSScrClass *sscr);
void teoSScrSubscribe(teoSScrClass *sscr, char *peer_name, uint16_t ev);
void teoSScrUnSubscribe(teoSScrClass *sscr, char *peer_name, uint16_t ev);
void teoSScrSend(teoSScrClass *sscr, uint16_t ev, void *data, size_t data_length, uint8_t cmd);
int teoSScrNumberOfSubscribers(teoSScrClass *sscr);

#ifdef	__cplusplus
}
#endif

#endif	/* SUBSCRIBE_H */
