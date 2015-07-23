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

/**
 * Initialize ksnMultiClass object
 * 
 * @param km
 */
ksnMultiClass *ksnMultiInit(ksnMultiData *md) {
    
    int i;
    ksnMultiClass *km = malloc(sizeof(ksnMultiClass));
    
    // Create network list
    km->list = pblListNewArrayList();
    km->num = md->num; // Set number of networks
   
    // Add networks from input data
    if(md != NULL && md->num) {
        
        for(i = 0; i < md->num; i++) {
            
            // Initialize network
            ksnetEvMgrClass *ke = ksnetEvMgrInitPort(md->argc, md->argv, 
                md->event_cb, READ_OPTIONS|READ_CONFIGURATION, md->ports[i]);
            
            // Set network parameters
            ke->km = km; // Pointer to multi net module
            ke->n_num = i; // Set network number
            ke->num_nets = md->num; // Set number of networks
            strncpy(ke->ksn_cfg.host_name, md->names[i], KSN_MAX_HOST_NAME); // Host name
            
            // Add to network list
            pblListAdd(km->list, ke);
            
            // Start network
            ksnetEvMgrRun(ke);
            
            // Start event manager 
            if(md->run && i == md->num - 1) ev_run(ke->ev_loop, 0);          
        }
        
    }
    
    return km;
}

/**
 * Destroy ksnMultiClass object
 * 
 * @param km
 */
void ksnMultiDestroy(ksnMultiClass *km) {
    
    if(km != NULL) {
        
        int i;
        
        // Destroy networks
        for(i = km->num-1; i >= 0; i--) {
            
            ksnetEvMgrClass *ke = pblListGet(km->list, i);
            ksnetEvMgrFree(ke, 2);
        }
        
        // Free network list
        pblListFree(km->list);
        
        // Free class memory
        free(km);
    }
}

/**
 * Get network by number
 * 
 * @param km
 * @param num
 * 
 * @return Pointer to ksnetEvMgrClass
 */
ksnetEvMgrClass *ksnMultiGet(ksnMultiClass *km, int num) {

    return pblListGet(km->list, num);
}

/**
 * Set number of networks to all modules list networks
 * 
 * @param km
 * @param num
 */
void ksnMultiSetNumNets(ksnMultiClass *km, int num) {
    
    int i;
    
    km->num = num; // Set new number of networks in ksnMultiClass
    
    // Set number of networks and serial number in every lists network
    for(i = 0; i < km->num; i++ ) {
        
        ksnetEvMgrClass *ke = pblListGet(km->list, i);
        ke->n_num = i;
        ke->num_nets = num;
    }
}

/**
 * Show list of networks
 * 
 * @param km
 * @return Pointer to string, should be free after use
 */
char *ksnMultiShowListStr(ksnMultiClass *km) {
    
    int i;
    char *str;

    #define add_line() \
    str = ksnet_sformatMessage(str, "%s" \
        "------------------------------------------------------------------\n",\
        str)


    str = ksnet_formatMessage("");
    add_line();

    str = ksnet_sformatMessage(str, "%s"
        "  # Name \t Port\n", str);
    add_line();
    
    for(i = 0; i < km->num; i++) {
        
        ksnetEvMgrClass *ke = pblListGet(km->list, i);
        
        str = ksnet_sformatMessage(str, "%s"
                "%3d %s%s%s\t %5d\n",
                str,
                
                // Number
                i+1,

                // Peer name
                getANSIColor(LIGHTGREEN), ke->ksn_cfg.host_name, getANSIColor(NONE),
                
                // Port
                ke->ksn_cfg.port
        );
    }    
    add_line();

    return str;
}

/**
 * Send command by name to peer
 *
 * @param km
 * @param to
 * @param cmd
 * @param data
 * @param data_len
 * @return
 */
ksnet_arp_data *ksnMultiSendCmdTo(ksnMultiClass *km, char *to, uint8_t cmd, 
        void *data, size_t data_len) {
    
    int i;
    ksnet_arp_data *arp = NULL;
    
    // Find peer in networks
    for(i = 0; i < km->num; i++) {
        
        // Get network and check its arp
        ksnetEvMgrClass *ke = pblListGet(km->list, i);
        arp = ksnetArpGet(ke->kc->ka, to);
        
        // Send to peer at network
        if(arp != NULL) {
            ksnCoreSendto(ke->kc, arp->addr, arp->port, cmd, data, data_len);
            break;
        }       
    }
        
    
    
    return 0;
}
