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
    
    size_t sscr_data_length = sizeof(teoSScrData) + data_length;
    teoSScrData *sscr_data = malloc(sscr_data_length);
    sscr_data->ev = ev;
    sscr_data->cmd = cmd;
    memcpy(sscr_data->data, data, data_length);
        
    size_t valueLength;
    teoSScrMapData *sscr_map_data = pblMapGet(sscr->map, &ev, sizeof(ev), 
            &valueLength);
    
    if(sscr_map_data != NULL) {
        
        int i, num = pblListSize(sscr_map_data->list);

        #ifdef DEBUG_KSNET
        ksnet_printf(
               &((ksnetEvMgrClass*)sscr->ke)->ksn_cfg,
               DEBUG,
               "%sSubscribe:%s "
               "Send CMD_SUBSCRIBE_ANSWER with event #%d, "
               "and data '%s', number of event subscribers: %d.\n",
                ANSI_LIGHTGREEN, ANSI_NONE,
                ev, (char*)data, num
        );
        #endif

        // Send event to subscribers
        for(i = 0; i < num; i++) {

            teoSScrData *sscr_list_data = pblListGet(sscr_map_data->list, i);

            // Send subscribe command to remote peer
            ksnCoreSendCmdto(((ksnetEvMgrClass*)sscr->ke)->kc,
                sscr_list_data->data, CMD_SUBSCRIBE_ANSWER, sscr_data,
                sscr_data_length);
        }
    }
    free(sscr_data);
}

/**
 * Sort list function
 * 
 * @param prev Previous element
 * @param next Next element
 * 
 * @return 
 */
static int list_compare (const void* prev, const void* next) {
    
//    if(((teoSScrListData*)prev)->ev < ((teoSScrListData*)next)->ev ) return -1;
//    else if(((teoSScrListData*)prev)->ev > ((teoSScrListData*)next)->ev ) return 1;
//    else return 0;
    
    return strcmp(((teoSScrData*)prev)->data, ((teoSScrData*)next)->data);
}

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
                teoSScrData *sscr_list_data = entry; 
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
 * Calculate number of subscribers
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
static void teoSScrFree(teoSScrData *element) {
    
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
 */
void teoSScrSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev) {
    
    #define add_data_to_list(sscr_list, peer_name) \
        teoSScrData *sscr_list_data = malloc(sizeof(teoSScrData) + \
            strlen(peer_name) + 1); \
        strcpy(sscr_list_data->data, peer_name); \
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
        add_data_to_list(sscr_map_data.list, peer_name);
        pblMapAdd(sscr->map, &ev, sizeof(ev), &sscr_map_data, 
                sizeof(sscr_map_data));
    }
    
    // Add new peer_name to existing event list
    else {
        
        // Find peer_name in list and add if absent
        if(find_in_list(sscr, peer_name, ev) == -1) {
        
            // Add peer_name to list
            add_data_to_list(sscr_data->list, peer_name);
        }
    }
    
    #ifdef DEBUG_KSNET
    ksnet_printf(
           &((ksnetEvMgrClass*)sscr->ke)->ksn_cfg,
           DEBUG, 
           "%sSubscribe:%s "
           "Peer '%s' was added to the Subscribers to event #%d. "
           "Number of subscribers: %d\n", 
            ANSI_LIGHTGREEN, ANSI_NONE,
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
            teoSScrData *sscr_data = pblListRemoveAt(sscr_map_data->list, idx);

            // Free element 
            if(sscr_data !=  (void*)-1) {
            
                #ifdef DEBUG_KSNET
                ksnet_printf(
                       &((ksnetEvMgrClass*)sscr->ke)->ksn_cfg,
                       DEBUG, 
                       "%sSubscribe:%s "
                       "Peer '%s' was removed from the Subscribers to event #%d. "
                       "Number of subscribers: %d\n", 
                        ANSI_LIGHTGREEN, ANSI_NONE,
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
        
        teoSScrSubscription(kco->ksscr, rd->from, *((uint16_t *)rd->data));
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
