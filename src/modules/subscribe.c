/** 
 * File:   subscribe.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on December 28, 2015, 1:31 PM
 */

/**
 * Subscribe module
 * 
 * Subscribe remote peers to host events events
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "subscribe.h"
#include "ev_mgr.h"
#include "utils/rlutil.h"

#define MODULE _ANSI_LIGHTGREEN "subscribe" _ANSI_NONE

typedef struct teoSScrMapData {
    
    PblList *list; ///< Pointer to map list
        
} teoSScrMapData;

/**
 * Initialize teoSScr module
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @return Pointer to created teoSScrClass or NULL if error occurred
 */
teoSScrClass *teoSScrInit(void *ke) {
    
    teoSScrClass *sscr = malloc(sizeof(teoSScrClass));
    
    if(sscr != NULL) {
        sscr->map = pblMapNewHashMap();
        sscr->ke = ke;
    }
    
    return sscr;
}

/**
 * Send event and it data to all subscribers
 * 
 * @param sscr Pointer to teoSScrClass
 * @param ev Event
 * @param data Event data
 * @param data_length Event data length
 * @param cmd Command, used for EV_K_RECEIVED event to show received command
 */
void teoSScrSend(teoSScrClass *sscr, uint16_t ev, void *data,
        size_t data_length, uint8_t cmd) {
    
    size_t valueLength;
    teoSScrMapData *sscr_map_data = pblMapGet(sscr->map, &ev, sizeof(ev), 
            &valueLength);
    
    if(sscr_map_data != NULL) {
        
        int i, num = pblListSize(sscr_map_data->list);

        if(num) {
            
            size_t sscr_data_length = sizeof(teoSScrData) + data_length;
            teoSScrData *sscr_out_data = malloc(sscr_data_length);
            sscr_out_data->ev = ev;
            sscr_out_data->cmd = cmd;
            memcpy(sscr_out_data->data, data, data_length);
        
            #ifdef DEBUG_KSNET
            ksn_printf(((ksnetEvMgrClass*)sscr->ke), MODULE, DEBUG_VV,
                   "send CMD_SUBSCRIBE_ANSWER with event #%d, "
                   "data length %d, number of subscribers to this event: %d\n",
                    ev, data_length, num
            );
            #endif

            // Send event to subscribers
            for(i = 0; i < num; i++) {

                teoSScrListData *sscr_list_data = pblListGet(sscr_map_data->list, i);

                if(!sscr_list_data->l0_f) {

                    // Send subscribe command to remote peer
                    ksnCoreSendCmdto(((ksnetEvMgrClass*)sscr->ke)->kc,
                        sscr_list_data->data, CMD_SUBSCRIBE_ANSWER, 
                        sscr_out_data, sscr_data_length);
                }
                else {
                    
                    // Send subscribe command to L0 client
                    ksnLNullSendToL0(sscr->ke, 
                        sscr_list_data->addr, sscr_list_data->port, 
                        sscr_list_data->data, strlen(sscr_list_data->data) + 1, 
                        CMD_SUBSCRIBE_ANSWER, 
                        sscr_out_data, sscr_data_length); 
                    
//                    printf("!!! Send to L0 client %s, server %s:%d\n", 
//                        sscr_list_data->data,
//                        sscr_list_data->addr, sscr_list_data->port);
                }
            }
            
            free(sscr_out_data);
        }
    }
}

///**
// * Sort list function
// * 
// * @param prev Previous element
// * @param next Next element
// * 
// * @return 
// */
//static int list_compare (const void* prev, const void* next) {
//    
////    if(((teoSScrListData*)prev)->ev < ((teoSScrListData*)next)->ev ) return -1;
////    else if(((teoSScrListData*)prev)->ev > ((teoSScrListData*)next)->ev ) return 1;
////    else return 0;
//    
//    return strcmp(((teoSScrListData*)prev)->data, ((teoSScrListData*)next)->data);
//}

/**
 * Find element with event and peer name in list
 * 
 * @param sscr Pointer to teoSScrClass
 * @param peer_name Remote peer name. (If empty - return first element with ev)
 * @param ev Event
 * 
 * @return Index number in the list or -1 if not found
 */
static int find_in_list(teoSScrClass *sscr, char *peer_name, uint16_t ev) {
    
    int retval = -1;
    
    size_t valueLength;    
    teoSScrMapData *sscr_map_data = pblMapGet(sscr->map, &ev, sizeof(ev), 
            &valueLength);
    if(sscr_map_data != NULL) {
        
        // Loop list and destroy all loaded modules
        PblIterator *it =  pblListIterator(sscr_map_data->list);
        if(it != NULL) {     

            int idx = 0;
            while(pblIteratorHasNext(it)) {        

                void *entry = pblIteratorNext(it);  
                teoSScrListData *sscr_list_data = entry; 
                if(peer_name[0] && !strcmp(sscr_list_data->data, peer_name)) {

                    retval = idx;
                    break;
                }

                idx++;
            }
            pblIteratorFree(it);
        }
    }
    
    return retval;
}

/**
 * Calculate number of subscriptions
 * 
 * @param sscr
 * @return 
 */
int teoSScrNumberOfSubscribers(teoSScrClass *sscr) {

    int retval = 0;
    
    PblIterator *it = pblMapIteratorNew(sscr->map);
    if(it != NULL) {
    
        while(pblIteratorHasNext(it)) {
            
            void *entry = pblIteratorNext(it);
            
            teoSScrMapData *sscr_map_data = pblMapEntryValue(entry);
            retval += pblListSize(sscr_map_data->list);
        }
        pblIteratorFree(it);
    }
    
    return retval;
}

/**
 * Free list element
 * 
 * @param sscr Pointer to teoSScrClass
 * @param element List element
 */
static void teoSScrFree(teoSScrListData *element) {
    
    if(element != NULL) {
        
        free(element);
    }
}

/**
 * Remote peer subscribed to event at this host
 * 
 * Remote peer 'peer_name' has subscribed to the event 'ev' at this host
 * 
 * @param sscr Pointer to teoSScrClass
 * @param peer_name Remote peer name
 * @param ev Event
 * @param arp Pointer to arp data if peer_name is L0 client, or NULL if it's a peer
 */
void teoSScrSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev, 
        ksnet_arp_data *arp) {
    
    // \todo In the add_data_to_list() Subscribe to L0 disconnect and remove 
    // disconnected clients    
    
    // \todo Remove all clients of disconnected L0 peer

    #define add_data_to_list(sscr_list, peer_name, e, c) \
        teoSScrListData *sscr_list_data = malloc(sizeof(teoSScrListData) + \
            strlen(peer_name) + 1); \
        strcpy(sscr_list_data->data, peer_name); \
        sscr_list_data->cmd = c; \
        sscr_list_data->ev = e; \
        if(arp == NULL) { \
            sscr_list_data->l0_f = 0; \
            sscr_list_data->addr[0] = 0; \
            sscr_list_data->port = 0; \
        } \
        else { \
            sscr_list_data->l0_f = 1; \
            strncpy(sscr_list_data->addr, arp->addr, sizeof(sscr_list_data->addr)); \
            sscr_list_data->port = arp->port; \
        } \
        pblListAdd(sscr_list, (void*)sscr_list_data)
    
    // Check event in map and create new record or update existing
    teoSScrMapData *sscr_data;
    size_t valueLength;
    
    // Create new record (new list) in map
    if((sscr_data = pblMapGet(sscr->map, &ev, 
            sizeof(ev), &valueLength)) == NULL) {
    
        // Add record to map
        teoSScrMapData sscr_map_data;
        sscr_map_data.list = pblListNewArrayList(); 
        add_data_to_list(sscr_map_data.list, peer_name, ev, 0);
        pblMapAdd(sscr->map, &ev, sizeof(ev), &sscr_map_data, 
                sizeof(sscr_map_data));
    }
    
    // Add new peer_name to existing event list
    else {
        
        // Find peer_name in list and add it if absent
        if(find_in_list(sscr, peer_name, ev) == -1) {
        
            // Add peer_name to list
            add_data_to_list(sscr_data->list, peer_name, ev, 0);
        }
    }
    
    #ifdef DEBUG_KSNET
    ksn_printf(((ksnetEvMgrClass*)sscr->ke), MODULE, DEBUG, 
           "peer '%s' was added to the Subscribers to event #%d, "
           "number of subscriptions: %d\n", 
            peer_name, ev, teoSScrNumberOfSubscribers(sscr)
    );
    #endif

    #undef add_data_to_list
}

/**
 * Remote peer unsubscribed from event at this host
 * 
 * @param sscr Pointer to teoSScrClass
 * @param peer_name Remote peer name
 * @param ev Event
 * 
 * @return True if this peer_name and event was present and successfully removed
 */
int teoSScrUnSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev) {
    
    int retval = 0;
    size_t valueLength;    
    teoSScrMapData *sscr_map_data = pblMapGet(sscr->map, &ev, sizeof(ev), 
            &valueLength);
    
    if(sscr_map_data != NULL) {

        // Find in list
        int idx = find_in_list(sscr, peer_name, ev);
        if(idx >= 0) {

            // Remove from list
            teoSScrListData *sscr_data = pblListRemoveAt(sscr_map_data->list, idx);

            // Free element 
            if(sscr_data !=  (void*)-1) {
            
                #ifdef DEBUG_KSNET
                ksn_printf(((ksnetEvMgrClass*)sscr->ke), MODULE, DEBUG, 
                       "peer \"%s\" was removed from the Subscribers to event #%d, "
                       "number of subscriptions: %d\n", 
                        sscr_data->data, sscr_data->ev, 
                        teoSScrNumberOfSubscribers(sscr)
                );
                #endif

                teoSScrFree(sscr_data);

                retval = 1;
            }
        }    
    }
    
    return retval;
}

/**
 * Unsubscribe peer_name from all events 
 * 
 * @param sscr
 * @param peer_name
 * @return 
 */
int teoSScrUnSubscriptionAll(teoSScrClass *sscr, char *peer_name) {

    int retval = 0;
    
    PblIterator *it = pblMapIteratorNew(sscr->map);
    if(it != NULL) {
    
        while(pblIteratorHasNext(it)) {
            
            void *entry = pblIteratorNext(it);
            
            uint16_t *ev = (uint16_t *) pblMapEntryKey(entry);
            retval += teoSScrUnSubscription(sscr, peer_name, *ev);
        }
        pblIteratorFree(it);
    }
    
    return retval;    
}

/**
 * Send command to subscribe this host to event at remote peer
 * 
 * @param sscr Pointer to teoSScrClass
 * @param peer_name Remote host name
 * @param ev Event
 */
void teoSScrSubscribe(teoSScrClass *sscr, char *peer_name, uint16_t ev) {
    
    // Send subscribe command to remote peer
    ksnCoreSendCmdto(((ksnetEvMgrClass*)sscr->ke)->kc, peer_name, 
            CMD_SUBSCRIBE, &ev, sizeof(ev));
}

/**
 * Send command to unsubscribe this host from event at remote peer
 * 
 * @param sscr Pointer to teoSScrClass
 * @param peer_name Remote host name
 * @param ev Event
 */
void teoSScrUnSubscribe(teoSScrClass *sscr, char *peer_name, uint16_t ev) {
    
    // Send unsubscribe command to remote peer
    ksnCoreSendCmdto(((ksnetEvMgrClass*)sscr->ke)->kc, peer_name, 
            CMD_UNSUBSCRIBE, &ev, sizeof(ev));
}

/**
 * Destroy teoSScr module
 * 
 * @param sscr Pointer to ksnCQueClass
 */
void teoSScrDestroy(teoSScrClass *sscr) {

    if(sscr != NULL) {
        
        PblIterator *it =  pblMapIteratorNew(sscr->map);
        if(it != NULL) {
            
            while(pblIteratorHasNext(it)) {
                
                void *entry = pblIteratorNext(it);
                teoSScrMapData *sscr_map_data = pblMapEntryValue(entry);

                // Loop list and destroy all loaded modules
                PblIterator *it_l =  pblListIterator(sscr_map_data->list);
                if(it_l != NULL) {     

                    while(pblIteratorHasNext(it_l)) {        

                        teoSScrFree(pblIteratorNext(it_l));              
                    }
                    pblIteratorFree(it_l);
                }        

                pblListFree(sscr_map_data->list);
            }
        }
        pblMapFree(sscr->map);
        free(sscr);
    }
}

/**
 * Process CMD_SUBSCRIBE, CMD_UNSUBSCRIBE and CMD_SUBSCRIBE_ANSWER commands
 * 
 * @param kco Pointer to ksnCommandClass
 * @param rd Pointer to ksnCorePacketData
 * @return  True if command is processed
 */
int cmd_subscribe_cb(ksnCommandClass *kco, ksnCorePacketData *rd) {
    
    #define kev ((ksnetEvMgrClass*) ((ksnCoreClass *) kco->kc)->ke)

    int processed = 0;
    
    if(rd->cmd == CMD_SUBSCRIBE) {
        
        uint16_t ev = *((uint16_t *)rd->data);
        if(rd->data_len >= 6 && !strncmp(rd->data, "TEXT:", 5)) {
            ev = atoi((char*)rd->data + 5);
//            printf("!!! %s\n", rd->data);
        }
        teoSScrSubscription(kco->ksscr, rd->from, ev, rd->l0_f ? rd->arp:NULL);
        
        // Send event callback
        if(kev->event_cb != NULL)
            kev->event_cb(kev, EV_K_SUBSCRIBED, (void*)rd, sizeof(*rd), NULL);
        
        processed = 1;        
    }
    
    else if(rd->cmd == CMD_UNSUBSCRIBE) {
        
        teoSScrUnSubscription(kco->ksscr, rd->from, *((uint16_t *)rd->data));
        processed = 1;
    }
    
    else if(rd->cmd == CMD_SUBSCRIBE_ANSWER) {

        // Send event callback
        if(kev->event_cb != NULL)
            kev->event_cb(kev, EV_K_SUBSCRIBE, (void*)rd, sizeof(*rd), NULL);
        
        processed = 1;
    }
    
    return processed;
    
    #undef kev
}
