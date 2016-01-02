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

/**
 * teoSScr class list or CMD_SUBSCRIBE_ANSWER data
 */
typedef struct teoSScrListData {
    
//    uint16_t ev; ///< Event to subscribe to
    char data[]; ///< Remote peer name in list or data in CMD_SUBSCRIBE_ANSWER
        
} teoSScrListData;

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
//        sscr->list = pblListNewArrayList();        
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
 */
void teoSScrSend(teoSScrClass *sscr, uint16_t ev, void *data, 
        size_t data_length) {
    
    size_t sscr_data_length = sizeof(teoSScrListData) + data_length;
    teoSScrListData *sscr_data = malloc(sscr_data_length);
    sscr_data->ev = ev;
    memcpy(sscr_data->data, data, data_length);
    
    
    int i, num = pblListSize(sscr->list);
    
    #ifdef DEBUG_KSNET
    ksnet_printf(
           &((ksnetEvMgrClass*)sscr->ke)->ksn_cfg,
           DEBUG, 
           "%sSubscribe:%s " 
           "Prepare to Send CMD_SUBSCRIBE_ANSWER with event #%d, "
           "and data '%s'. Number of subscribers: %d.\n", 
            ANSI_LIGHTGREEN, ANSI_NONE,
            ev, (char*)data, num
    );
    #endif
    
    // Send event to subscribers
    for(i = 0; i < num; i++) {
        
        teoSScrListData *sscr_list_data = pblListGet(sscr->list, i);
        if(sscr_list_data->ev == ev) {
            
            // Send subscribe command to remote peer
            ksnCoreSendCmdto(((ksnetEvMgrClass*)sscr->ke)->kc, 
                sscr_list_data->data, CMD_SUBSCRIBE_ANSWER, sscr_data, 
                sscr_data_length);    
        }
    }
    
    free(sscr_data);
}

/**
 * Sort the list function
 * 
 * @param prev Previous element
 * @param next Next element
 * 
 * @return 
 */
static int list_compare (const void* prev, const void* next) {
    
    if(((teoSScrListData*)prev)->ev < ((teoSScrListData*)next)->ev ) return -1;
    else if(((teoSScrListData*)prev)->ev > ((teoSScrListData*)next)->ev ) return 1;
    else return 0;
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
    
    teoSScrMapData *sscr_map_data = pblMapGet(sscr->map, &ev, sizeof(ev), &valueLength);
    if(sscr_map_data != NULL) {
        
        // Loop list and destroy all loaded modules
        PblIterator *it =  pblListIterator(sscr_map_data->list);
        if(it != NULL) {     

            int idx = 0;
            while(pblIteratorHasNext(it)) {        

                teoSScrListData *sscr_data = pblIteratorNext(it);  
//                if(sscr_data->ev == ev) {

                    if(!peer_name[0] || !strcmp(sscr_data->data, peer_name)) {

                        retval = idx;
                        break;
                    }
//                }    

                idx++;
            }
            pblIteratorFree(it);
        }
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
 */
void teoSScrSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev) {
    
//    teoSScrData *sscr_data = malloc(sizeof(teoSScrData) + strlen(peer_name) + 1);
//    sscr_data->ev = ev;
//    strcpy(sscr_data->data, peer_name);
//    pblListAdd(sscr->list, (void*)sscr_data);
//    
//    pblListSort(sscr->list, list_compare);
    
    #define add_data_to_list(sscr_list, peer_name) \
        teoSScrListData *sscr_list_data = malloc(sizeof(teoSScrListData) + strlen(peer_name) + 1); \
        strcpy(sscr_list_data->data, peer_name); \
        pblListAdd(sscr_list, (void*)sscr_list_data)

    
    // Check event in map and create new record or update existing
    teoSScrMapData *sscr_data;
    size_t valueLength;
    
    // Create new record (new list) in map
    if((sscr_data = pblMapGet(sscr->map, &ev, sizeof(ev), &valueLength)) == NULL) {
    
        // Add record to map
        teoSScrMapData sscr_map_data;
        sscr_map_data.list = pblListNewArrayList(); 
//        teoSScrListData *sscr_list_data = malloc(sizeof(teoSScrListData) + strlen(peer_name) + 1);
//        strcpy(sscr_list_data->data, peer_name);
//        pblListAdd(sscr_map_data.list, (void*)sscr_list_data);
        add_data_to_list(sscr_map_data.list, peer_name);
        pblMapAdd(sscr->map, &ev, sizeof(ev), &sscr_map_data, sizeof(sscr_map_data));
    }
    // Add new peer_name to existing event list
    else {
        
        // Find peer_name in list and add if absent
        if(find_in_list(sscr, peer_name, ev) == -1) {
        
            // Add peer_name to list
//            teoSScrListData *sscr_list_data = malloc(sizeof(teoSScrListData) + strlen(peer_name) + 1);
//            strcpy(sscr_list_data->data, peer_name);
//            pblListAdd(sscr_list, (void*)sscr_list_data);
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
            peer_name, ev, 0 // \todo pblListSize(sscr->list)
    );
    #endif
}


/**
 * Remote peer unsubscribed from event at this host
 * 
 * @param sscr Pointer to teoSScrClass
 * @param peer_name Remote peer name
 * @param ev Event
 */
void teoSScrUnSubscription(teoSScrClass *sscr, char *peer_name, uint16_t ev) {
    
    // Find in list
    int idx = find_in_list(sscr, peer_name, ev);
    if(idx >= 0) {
        
        // Remove from list
        teoSScrListData *sscr_data = pblListRemoveAt(sscr->list, idx);
        
        // Free element 
        if(sscr_data !=  (void*)-1) teoSScrFree(sscr_data);
    }
}

/**
 * This host subscribe to event at remote peer
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
 * This host unsubscribe from event at remote peer
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
        
        // Loop list and destroy all loaded modules
        PblIterator *it =  pblListIterator(sscr->list);
        if(it != NULL) {     
            
            while(pblIteratorHasNext(it)) {        

              teoSScrFree(pblIteratorNext(it));              
            }
            pblIteratorFree(it);
        }        
        
        pblListFree(sscr->list);
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
