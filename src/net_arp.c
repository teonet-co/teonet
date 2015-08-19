/**
 * File:   net_arp.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on April 12, 2015, 7:19 PM
 *
 * KSNet ARP Table manager
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "net_tr-udp_.h"
#include "utils/rlutil.h"
#include "utils/utils.h"


/******************************************************************************/
/* KSNet ARP functions                                                        */
/*                                                                            */
/******************************************************************************/

/**
 * Initialize ARP table
 */
ksnetArpClass *ksnetArpInit(void *ke) {

    #define kev ((ksnetEvMgrClass*)(ke))

    ksnetArpClass *ka = malloc(sizeof(ksnetArpClass));
    ka->map = pblMapNewHashMap();
    ka->ke = ke;
    
    ksnetArpAddHost(
        ka,
        kev->ksn_cfg.host_name,
        "0.0.0.0",        
        kev->kc->port
    );

    return ka;
    
    #undef kev 
}

/**
 * Destroy ARP table
 */
void ksnetArpDestroy(ksnetArpClass *ka) {

    pblMapFree(ka->map);
    free(ka);
}

/**
 * Get ARP table data by Peer Name
 *
 * @param name
 * @return Pointer to ARP data or NULL if not found
 */
ksnet_arp_data * ksnetArpGet(ksnetArpClass *ka, char *name) {

    size_t val_len;
    return (ksnet_arp_data *) pblMapGetStr(ka->map, name, &val_len);
}

/**
 * Add or update record in KSNet Peer ARP table
 *
 * @param ka
 * @param name
 * @param data
 */
void ksnetArpAdd(ksnetArpClass *ka, char* name, ksnet_arp_data *data) {

    pblMapAdd(
        ka->map,
        (void *) name, strlen(name) + 1,
        data, sizeof(*data)
    );
}

/**
 * Add (or update) this host to ARP table
 *
 * @param name
 */
void ksnetArpAddHost(ksnetArpClass *ka, char* name, char *addr, int port) {

    ksnet_arp_data arp;
    
    memset(&arp, 0, sizeof(arp));
    arp.mode = -1;
    strncpy(arp.addr, addr, sizeof(arp.addr));
    arp.port = port;

    ksnetArpAdd(ka, name, &arp);
}

/**
 * Change port at existing arp data associated with key (per name)
 * 
 * @param ka
 * @param name
 * @param port
 * @return Return NULL if name not found in the map
 */
void *ksnetArpSetHostPort(ksnetArpClass *ka, char* name, int port) {
    
    size_t valueLength;
    
    ksnet_arp_data* arp = pblMapGetStr(ka->map, name, &valueLength); 
    
    if(arp != NULL) {
        arp->port = port;
    }
    
    return arp;
}

/**
 * Remove Peer from the ARP table
 *
 * @param name
 * @return Pointer to previously associated value or NULL if not found
 */
ksnet_arp_data * ksnetArpRemove(ksnetArpClass *ka, char* name) {

    size_t var_len = 0;
    //printf("ARP map remove '%s' ... ", name);
    ksnet_arp_data *arp = pblMapRemoveStr(ka->map, name, &var_len);
    if(arp != (void*)-1) {
        printf("removed %s:%d\n", arp->addr, arp->port);
        ksnTRUDPresetAddr( ((ksnetEvMgrClass*) ka->ke)->kc->ku, arp->addr, 
                arp->port, 1);
    }
    //else printf("not found\n");
    
    // If not found
    if(arp == (void*)-1) arp = NULL;
    
    return arp;
}

/**
 * Get all known peer. Send it too fnd_peer_cb callback
 *
 * @param ka
 * @param peer_callback int peer_callback(ksnetArpClass *ka, char *peer_name, ksnet_arp_data *arp_data, void *data)
 * @param data
 */
int ksnetArpGetAll(ksnetArpClass *ka,
        int (*peer_callback)(ksnetArpClass *ka, char *peer_name, 
            ksnet_arp_data *arp_data, void *data), 
        void *data) {

    int retval = 0;

    PblIterator *it =  pblMapIteratorNew(ka->map);
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = pblMapEntryKey(entry);
            ksnet_arp_data *arp_data = pblMapEntryValue(entry);

            if(arp_data->mode >= 0) { // && (!child_only || !data->direct_con)) {

                if(peer_callback(ka, name, arp_data, data)) {

                    retval = 1;
                    break;
                }
            }
        }
        pblIteratorFree(it);
    }

    return retval;
}

typedef struct find_arp_data {
    
  __CONST_SOCKADDR_ARG addr;
  ksnet_arp_data *arp_data;
    
} find_arp_data;

int find_arp_by_addr_cb(ksnetArpClass *ka, char *peer_name, 
            ksnet_arp_data *arp_data, void *data) {
    
    int retval = 0;
    find_arp_data *fa = data;
    
    if(ntohs(((struct sockaddr_in *) fa->addr)->sin_port) == arp_data->port &&
       !strcmp(inet_ntoa(((struct sockaddr_in *) fa->addr)->sin_addr), 
            arp_data->addr)) {
        
        fa->arp_data = arp_data;
        
        retval = 1;
    }
    
    return retval;
}

/**
 * Find ARP data by address
 * 
 * @param ka
 * @param addr
 * 
 * @return  Pointer to ARP data
 */
ksnet_arp_data *ksnetArpFindByAddr(ksnetArpClass *ka, __CONST_SOCKADDR_ARG addr) {

    find_arp_data fa;
    fa.addr = addr;
    fa.arp_data = NULL;
    
    //char key[KSN_BUFFER_SM_SIZE];
    //ksnTRUDPkeyCreate(NULL, addr, key, KSN_BUFFER_SM_SIZE);
    
    if(ka != NULL && ksnetArpGetAll(ka, find_arp_by_addr_cb, (void*) &fa)) {
        
        // ARP by address was found
        //printf("ARP by address %s was found\n", key);
        
    } else {
        
        // ARP by address %s not found
        //printf("ARP by address %s not found\n", key);
        
    }
    
    return fa.arp_data;
}

/**
 * Show (return string) KSNet ARP table
 *
 * @param ka
 * @return
 */
char *ksnetArpShowStr(ksnetArpClass *ka) {

    char *str;

    str = ksnet_formatMessage(
        "------------------------------------------------------------------\n");

    str = ksnet_sformatMessage(str, "%s"
        "  # Peer \t Mod \t IP \t\t   Port\t  Trip time  Rx/Tx\n", str);

    str = ksnet_sformatMessage(str, "%s"
        "------------------------------------------------------------------\n",
        str);

    PblIterator *it = pblMapIteratorNew(ka->map);
    int num = 0;
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = pblMapEntryKey(entry);
            ksnet_arp_data *data = pblMapEntryValue(entry);
            char *last_triptime = ksnet_formatMessage("%7.3f", data->last_triptime);

            str = ksnet_sformatMessage(str, "%s"
                "%3d %s%s%s\t %3d \t %-15s  %5d\t %7s %s \t %s%s%s \n",
                str,

                // Number
                ++num,

                // Peer name
                getANSIColor(LIGHTGREEN), name, getANSIColor(NONE),

                // Index
                data->mode,

                // IP
                data->addr,

                // Port
                data->port,

                // Trip time
                data->mode < 0 ? "" : last_triptime,

                // Trip time type (ms)
                data->mode < 0 ? "" : "ms",

                // Rx/Tx
                "", //(data->idx >= 0 && data->direct_con ? itoa( (&kn->host->peers[data->idx])->incomingDataTotal) : ""),
                "", // (data->idx >= 0 && data->direct_con ? "/" : ""),
                "" //(data->idx >= 0 && data->direct_con ? itoa( (&kn->host->peers[data->idx])->outgoingDataTotal) : "")
            );
            free(last_triptime);
        }
        pblIteratorFree(it);
    }
    //ksnet_printf(&((ksnetEvMgrClass*)kn->ke)->ksn_cfg, MESSAGE,
    str = ksnet_sformatMessage(str, "%s"
        "------------------------------------------------------------------\n",
        str);


    return str;
}

/**
 * Show KSNet ARP table
 *
 * @param ka
 *
 * @return Number of lines in shown text
 */
int ksnetArpShow(ksnetArpClass *ka) {

    int num_line = 0;
    char *str = ksnetArpShowStr(ka);

    ksnet_printf(&((ksnetEvMgrClass*)ka->ke)->ksn_cfg, MESSAGE, "%s", str);
    num_line = calculate_lines(str);

    free(str);

    return num_line;
}
