/** 
 * File:   net_multi.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Module to manage multi teonet networks (host, client or proxy)
 * 
 * Created on July 23, 2015, 11:46 AM
 */

#include <stdlib.h>
#include <string.h>

#include "net_multi.h"
#include "utils/rlutil.h"
#include "utils/teo_memory.h"

static void ksnMultiUpdateCountNetworks(ksnMultiClass *km, int num);

/**
 * Initialize ksnMultiClass object
 * 
 * @param md Pointer to ksnMultiData
 */
ksnMultiClass *ksnMultiInit(ksnMultiData *md, void *user_data) {
    
    int i;
    ksnMultiClass *km = teo_malloc(sizeof(ksnMultiClass));
    
    // Create network list
    km->list = pblMapNewHashMap();
    km->num = md->num; // Set number of networks
   
    // Add networks from input data
    if(md != NULL && md->num) {
        
        for(i = 0; i < md->num; i++) {
            
            // Initialize network
            ksnetEvMgrClass *ke = ksnetEvMgrInitPort(md->argc, md->argv, 
                md->event_cb, READ_OPTIONS|READ_CONFIGURATION, md->ports[i], 
                user_data);
            
            // Set network parameters
            ke->km = km; // Pointer to multi net module
            ke->n_num = i; // Set network number
            ke->num_nets = md->num; // Set number of networks
            strncpy(ke->ksn_cfg.host_name, md->names[i], KSN_MAX_HOST_NAME - strlen(ke->ksn_cfg.host_name)); // Host name
            strncpy(ke->ksn_cfg.network, md->networks[i], KSN_BUFFER_SM_SIZE/2 - strlen(ke->ksn_cfg.network)); // Network name
            read_config(&ke->ksn_cfg, ke->ksn_cfg.port); // Read configuration file parameters

            // Add to network list
            pblMapAdd(km->list, (void *)ke->ksn_cfg.network, strlen(ke->ksn_cfg.network)+1,
                    &ke, sizeof(ke));
            
            // Start network
            ksnetEvMgrRun(ke);
            
            // Start event manager
            if(md->run && i == md->num - 1) ev_run(ke->ev_loop, 0);
        }
        
    }
    
    return km;
}


void teoMultiAddNet(ksnMultiClass *km, char *host, char *network) {
    ksnetEvMgrClass *ke_last = teoMultiGetByNumber(km, km->num-1);

    // We need to update count of networks for old networks
    ksnMultiUpdateCountNetworks(km, km->num + 1);

    ksnetEvMgrClass *ke_new = ksnetEvMgrInitPort(ke_last->argc, ke_last->argv,
                ke_last->event_cb, READ_OPTIONS|READ_CONFIGURATION, ke_last->ksn_cfg.port+3,
                NULL);

    // Set network parameters
    ke_new->km = km; // Pointer to multi net module
    ke_new->n_num = km->num - 1; // Set network number
    ke_new->num_nets = km->num; // Set number of networks
    strncpy(ke_new->ksn_cfg.host_name, host, KSN_MAX_HOST_NAME - strlen(ke_new->ksn_cfg.host_name)); // Host name
    strncpy(ke_new->ksn_cfg.network, network, KSN_BUFFER_SM_SIZE/2 - strlen(ke_new->ksn_cfg.network)); // Network name
    read_config(&ke_new->ksn_cfg, ke_new->ksn_cfg.port); // Read configuration file parameters

    // Add to network list
    pblMapAdd(km->list, (void *)ke_new->ksn_cfg.network, strlen(ke_new->ksn_cfg.network)+1,
            &ke_new, sizeof(ke_new));

    // Start network
    ksnetEvMgrRun(ke_new);

    // // Start event manager
    // ev_run(ke_new->ev_loop, 0);
}


void teoMultiRemoveNet(ksnMultiClass *km, char *network) {

    ksnetEvMgrClass **ke = pblMapRemoveStr(km->list, network, NULL);

    if(ke == (void *)-1) return;

    ksnetEvMgrStop(*ke);
    ksnetEvMgrFree(*ke, 2);
    free(ke);

    ksnMultiUpdateCountNetworks(km, km->num - 1);
}


/**
 * Destroy ksnMultiClass object and networks
 * 
 * @param km
 */
void ksnMultiDestroy(ksnMultiClass *km) {
    
    if(!km) return;

    PblIterator *it = pblMapIteratorNew(km->list);
    if(!it) return;

    int argc = 0;
    char **argv = NULL;

    while(pblIteratorHasNext(it)) {
        void *entry = pblIteratorNext(it);
        ksnetEvMgrClass **ke = pblMapEntryValue(entry);
        ksnetEvMgrStop(*ke);
        argc = (*ke)->argc;
        argv = (*ke)->argv;
        ksnetEvMgrFree(*ke, 2);
    }

    pblMapFree(km->list);
    free(km);

    // Restart application if need it
    ksnetEvMgrRestart(argc, argv);
}


/**
 * Get network by number
 * 
 * @param km
 * @param num
 * 
 * @return Pointer to ksnetEvMgrClass
 */
ksnetEvMgrClass *teoMultiGetByNumber(ksnMultiClass *km, int number) {
    PblIterator *it = pblMapIteratorNew(km->list);
    if(!it) return NULL;

    while(pblIteratorHasNext(it)) {
        void *entry = pblIteratorNext(it); 
        ksnetEvMgrClass **ke = pblMapEntryValue(entry);
        if ((*ke)->n_num == number) return *ke;
    }

    return NULL;
}


/**
 * Get network by network name
 * 
 * @param km
 * @param network_name
 * 
 * @return Pointer to ksnetEvMgrClass or NULL if not found
 */
ksnetEvMgrClass *teoMultiGetByNetwork(ksnMultiClass *km, char *network_name) {
    ksnetEvMgrClass **ke = pblMapGetStr(km->list, network_name, NULL);
    if(ke) return *ke;
    return NULL;
}


/**
 * Update number of networks to all old networks
 *
 * @param km
 * @param num
 */
static void ksnMultiUpdateCountNetworks(ksnMultiClass *km, int num) {

    PblIterator *it = pblMapIteratorNew(km->list);
    if(!it) return;

    km->num = num; // Set new number of networks in ksnMultiClass

    while(pblIteratorHasNext(it)) {
        void *entry = pblIteratorNext(it);
        ksnetEvMgrClass **ke = pblMapEntryValue(entry);
        (*ke)->num_nets = num;
    }
}

/**
 * Set number of networks to all modules list networks
 * 
 * @param km
 * @param num
 */
void ksnMultiSetNumNets(ksnMultiClass *km, int num) {

    PblIterator *it = pblMapIteratorNew(km->list);
    if(!it) return;

    km->num = num; // Set new number of networks in ksnMultiClass

    int idx = 0;

    while(pblIteratorHasNext(it)) {
        void *entry = pblIteratorNext(it);
        ksnetEvMgrClass **ke = pblMapEntryValue(entry);
        (*ke)->n_num = idx++;
        (*ke)->num_nets = num;
    }
}

/**
 * Show list of networks
 * 
 * @param km
 * @return Pointer to string, should be free after use
 */
char *ksnMultiShowListStr(ksnMultiClass *km) {

    char *str;

    #define add_line() \
    str = ksnet_sformatMessage(str, "%s" \
        "------------------------------------------------------------------\n",\
        str)

    str = ksnet_formatMessage("");
    add_line();

    str = ksnet_sformatMessage(str, "%s"
        "  # Name                 Network               Port\n ", str);
    add_line();

    PblIterator *it = pblMapIteratorNew(km->list);
    if(!it) return NULL;

    while(pblIteratorHasNext(it)) {
        void *entry = pblIteratorNext(it);
        char *network_name = pblMapEntryKey(entry);
        ksnetEvMgrClass **ke = pblMapEntryValue(entry);

        str = ksnet_sformatMessage(str, "%s"
                "%3d %s%-20s%s %s%-20s%s %5d\n",
                str,
                // Number
                (*ke)->n_num+1,
                // Peer name
                getANSIColor(LIGHTGREEN), (*ke)->ksn_cfg.host_name, getANSIColor(NONE),
                getANSIColor(LIGHTCYAN), network_name, getANSIColor(NONE),
                // Port
                (*ke)->ksn_cfg.port
        );
    }

    add_line();

    return str;
}


/**
 * Send command by name and by network
 *
 *
 * @return Pointer to ksnet_arp_data or NULL if not found
 */
ksnet_arp_data *teoMultiSendCmdToNet(ksnMultiClass *km, char *peer, char *network,
        uint8_t cmd, void *data, size_t data_len) {

    ksnetEvMgrClass **ke = pblMapGetStr(km->list, network, NULL);
    ksnet_arp_data *arp = (ksnet_arp_data *)ksnetArpGet((*ke)->kc->ka, peer);
    if(!arp) return NULL;

    ksnCoreSendto((*ke)->kc, arp->addr, arp->port, cmd, data, data_len);
    return arp;
}


/**
 * \TODO: It will be new function for broadcast sending
 * Send command by name to peer
 *
 * @param km Pointer to ksnMultiClass
 * @param to Recipient peer name
 * @param cmd Command
 * @param data Command data
 * @param data_len Data length
 *
 * @return Pointer to ksnet_arp_data or NULL if not found
 */
ksnet_arp_data *ksnMultiSendCmdTo(ksnMultiClass *km, char *to, uint8_t cmd, 
        void *data, size_t data_len) {

    // \TODO: create body here
    // int i;
    ksnet_arp_data *arp = NULL;

    return arp;
}
