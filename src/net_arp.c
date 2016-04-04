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
#include "tr-udp_.h"
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
    
    ksnetArpAddHost(ka);

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
 * @param ka Pointer to ksnetArpClass
 * @param name Peer name
 * @return Pointer to ARP data or NULL if not found
 */
ksnet_arp_data * ksnetArpGet(ksnetArpClass *ka, char *name) {

    size_t val_len;
    return (ksnet_arp_data *) pblMapGetStr(ka->map, name, &val_len);
}


/**
 * Returns the number of entries in arp map.
 * 
 * @param ka Pointer to ksnetArpClass
 * @return The number of entries in arp map.
 */
int ksnetArpSize(ksnetArpClass *ka) {
    
    return pblMapSize(ka->map);
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
 * @param ka
 */
void ksnetArpAddHost(ksnetArpClass *ka) { 

    ksnetEvMgrClass *ke = ka->ke;
    ksnet_arp_data arp;
    
    char* name = ke->ksn_cfg.host_name;
    char *addr = (char*)localhost; //"0.0.0.0";
    int port = ke->kc->port;
    
    memset(&arp, 0, sizeof(arp));
    arp.connected_time = ev_now(ke->ev_loop); //ksnetEvMgrGetTime(ke);
    strncpy(arp.addr, addr, sizeof(arp.addr));
    arp.port = port;
    arp.mode = -1;

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
 * @param ka Pointer to ksnetArpClass
 * @param name Peer name to remove
 * @return Pointer to previously associated value or NULL if not found
 */
ksnet_arp_data * ksnetArpRemove(ksnetArpClass *ka, char* name) {

    size_t var_len = 0;
    char* peer_name = strdup(name);
    
    // Remove from ARP table
    ksnet_arp_data *arp = pblMapRemoveStr(ka->map, name, &var_len);
    
    // If removed successfully
    if(arp != (void*)-1) {
        
        // Remove peer from TR-UDP module
        ksnTRUDPresetAddr( ((ksnetEvMgrClass*) ka->ke)->kc->ku, arp->addr, 
                arp->port, 1);
        
        // Remove from Stream module
        ksnStreamClosePeer(((ksnetEvMgrClass*) ka->ke)->ks, peer_name);
    }    
    
    // If not found
    if(arp == (void*)-1) arp = NULL;
    
    free(peer_name);
    
    return arp;
}

/**
 * Remove all records instead host from ARP table
 * 
 * Free ARP Hash map, create new and add this host to created ARP Hash map
 * 
 * @param ka
 */
void ksnetArpRemoveAll(ksnetArpClass *ka) {
        
    ksnetEvMgrClass *ke = ka->ke;
    
    pblMapFree(ka->map);
    ke->ksn_cfg.r_host_name[0] = '\0';
    ka->map = pblMapNewHashMap();    
    ksnetArpAddHost(ka);
    ksnTRUDPremoveAll(ke->kc->ku);
}

/**
 * Get all known peer without current host. Send it too fnd_peer_cb callback
 *
 * @param ka
 * @param peer_callback int peer_callback(ksnetArpClass *ka, char *peer_name, ksnet_arp_data *arp_data, void *data)
 * @param data
 * @param flag Include this host if true
 */
int ksnetArpGetAll_(ksnetArpClass *ka,
        int (*peer_callback)(ksnetArpClass *ka, char *peer_name, 
            ksnet_arp_data *arp_data, void *data), 
        void *data, int flag) {

    int retval = 0;

    PblIterator *it =  pblMapIteratorNew(ka->map);
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = pblMapEntryKey(entry);
            ksnet_arp_data *arp_data = pblMapEntryValue(entry);

            if(flag || arp_data->mode >= 0) { 

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

/**
 * Get all known peer without current host. Send it too fnd_peer_cb callback
 *
 * @param ka
 * @param peer_callback int peer_callback(ksnetArpClass *ka, char *peer_name, ksnet_arp_data *arp_data, void *data)
 * @param data
 */
inline int ksnetArpGetAll(ksnetArpClass *ka,
        int (*peer_callback)(ksnetArpClass *ka, char *peer_name, 
            ksnet_arp_data *arp_data, void *data), 
        void *data) {
       
    return ksnetArpGetAll_(ka, peer_callback, data, 0);
}

/**
 * Get all known peer without current host. Send it too fnd_peer_cb callback
 *
 * @param ka
 * @param peer_callback int peer_callback(ksnetArpClass *ka, char *peer_name, ksnet_arp_data *arp_data, void *data)
 * @param data
 */
inline int ksnetArpGetAllH(ksnetArpClass *ka,
        int (*peer_callback)(ksnetArpClass *ka, char *peer_name, 
            ksnet_arp_data *arp_data, void *data), 
        void *data) {
    
    return ksnetArpGetAll_(ka, peer_callback, data, 1);
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
    
    if(ka != NULL && ksnetArpGetAllH(ka, find_arp_by_addr_cb, (void*) &fa)) {
        
        // ARP by address was found
        //printf("ARP by address %s:%d was found\n", 
        //            inet_ntoa(((struct sockaddr_in *) addr)->sin_addr),
        //            ntohs(((struct sockaddr_in *) addr)->sin_port));
        
    } else {
        
        // ARP by address %s not found
        //printf("ARP by address %s:%d not found\n", 
        //            inet_ntoa(((struct sockaddr_in *) addr)->sin_addr),
        //            ntohs(((struct sockaddr_in *) addr)->sin_port));
        
    }
    
    return fa.arp_data;
}

/**
 * Return ARP table in digital format
 * 
 * @param ka Pointer to the ksnetArpClass
 * @return Return pointer to the ksnet_arp_data_ar data with ARP table data. 
 * Should be free after use.
 */
ksnet_arp_data_ar *ksnetArpShowData(ksnetArpClass *ka) {
    
    uint32_t length = pblMapSize(ka->map);
    ksnet_arp_data_ar *data_ar = malloc(sizeof(ksnet_arp_data_ar) + length * sizeof(data_ar->arp_data[0]));
    data_ar->length = length;
    
    PblIterator *it = pblMapIteratorNew(ka->map);
    int i = 0;
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = pblMapEntryKey(entry);
            ksnet_arp_data *data = pblMapEntryValue(entry);
            strncpy(data_ar->arp_data[i].name, name, sizeof(data_ar->arp_data[i].name));
            memcpy(&data_ar->arp_data[i].data, data, sizeof(data_ar->arp_data[i].data));
            data_ar->arp_data[i].data.connected_time = ksnetEvMgrGetTime(ka->ke) - data_ar->arp_data[i].data.connected_time;
            i++;
        }
    }
    
    return data_ar;
}

/**
 * Convert peers data to JSON
 * 
 * @param peers_data Pointer to ksnet_arp_data_ar
 * @param peers_data_json_len [out] Result json string length
 * @return 
 */
char *ksnetArpShowDataJson(ksnet_arp_data_ar *peers_data, 
        size_t *peers_data_json_len) {
    
    ksnet_arp_data_ar *arp_data_ar = (ksnet_arp_data_ar *) peers_data;
    size_t data_str_len = sizeof(arp_data_ar->arp_data[0]) * 2 * arp_data_ar->length;
    char *data_str = malloc(data_str_len);
    int ptr = snprintf(data_str, data_str_len, "{ \"length\": %d, \"arp_data_ar\": [ ", arp_data_ar->length);
    int i = 0;
    for(i = 0; i < arp_data_ar->length; i++) {
        ptr += snprintf(data_str + ptr, data_str_len - ptr,
                "%s{ "
                "\"name\": \"%s\", "
                "\"mode\": %d, "
                "\"addr\": \"%s\", "
                "\"port\": %d, "
                "\"triptime\": %.3f,"
                "\"uptime\": %.3f"
                " }", 
                i ? ", " : "", 
                arp_data_ar->arp_data[i].name,
                arp_data_ar->arp_data[i].data.mode,
                arp_data_ar->arp_data[i].data.addr,
                arp_data_ar->arp_data[i].data.port,
                arp_data_ar->arp_data[i].data.last_triptime,
                arp_data_ar->arp_data[i].data.connected_time // uptime
        );
    }
    snprintf(data_str + ptr,  data_str_len - ptr, " ] }");
    
    if(peers_data_json_len != NULL) *peers_data_json_len = strlen(data_str);
    
    return data_str;
}

/**
 * Return size of ksnet_arp_data_ar data
 * 
 * @param peers_data
 * @return Size of ksnet_arp_data_ar data
 */
inline size_t ksnetArpShowDataLength(ksnet_arp_data_ar *peers_data) {
    
    return peers_data != NULL ?
        sizeof(ksnet_arp_data_ar) + peers_data->length * sizeof(peers_data->arp_data[0])
            : 0;
}


/**
 * Show (return string) with KSNet ARP table header
 * 
 * @param header_f Header flag. If 1 - return Header, if 0 - return Footer
 * @return 
 */
char *ksnetArpShowHeader(int header_f) {

    char *str;
    const char *div = "-------------------------------------------------------"
                      "-----\n";

    str = ksnet_formatMessage(div);

    // Header part
    if(header_f) {
        
        str = ksnet_sformatMessage(str, "%s"
            "  # Peer \t Mod | IP \t\t| Port | Trip time\n", 
            str);

        str = ksnet_sformatMessage(str, "%s%s", str, div);
    }

    return str;
}

/**
 * Show (return string) one record of KSNet ARP table
 * 
 * @param num Record number
 * @param name Peer name
 * @param data Pointer to ksnet_arp_data structure
 * @return String with formated ARP table line. Should be free after use
 */
char *ksnetArpShowLine(int num, char *name, ksnet_arp_data* data) {
    
    // Format last trip time
    char *last_triptime = ksnet_formatMessage("%7.3f", data->last_triptime);
    
    char *str = ksnet_formatMessage(
        "%3d %s%s%s\t %3d   %-15s  %5d   %7s %s\n",

        // Number
        num,

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

        // ARP Trip time type (ms)
        data->mode < 0 ? "" : "ms",

        // TCP Proxy last trip time type (ms)
        ""
    );
    
    free(last_triptime);
            
    return str;
}

/**
 * Show (return string) KSNet ARP table
 *
 * @param ka
 * @return String with formated ARP table. Should be free after use
 */
char *ksnetArpShowStr(ksnetArpClass *ka) {

    char *str;
    const char *div = "-------------------------------------------------------"
                      "--------------------------\n";

    str = ksnet_formatMessage(div);

    str = ksnet_sformatMessage(str, "%s"
        "  # Peer \t Mod | IP \t\t| Port | Trip time | TR-UDP trip time\n", 
        str);

    str = ksnet_sformatMessage(str, "%s%s", str, div);

    PblIterator *it = pblMapIteratorNew(ka->map);
    int num = 0;
    if(it != NULL) {

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = pblMapEntryKey(entry);
            ksnet_arp_data *data = pblMapEntryValue(entry);
            char *last_triptime = ksnet_formatMessage("%7.3f", 
                    data->last_triptime);
            
            // Get TR-UDP ip map data by key
            size_t val_len;
            size_t key_len = KSN_BUFFER_SM_SIZE;
            char key[key_len];
            key_len = snprintf(key, key_len, "%s:%d", data->addr, data->port);           
            ip_map_data *ip_map_d = pblMapGet(
                    ((ksnetEvMgrClass*)ka->ke)->kc->ku->ip_map, key, key_len, 
                    &val_len);
            
            // Last trip time
            char *tcp_last_triptime = ip_map_d != NULL ? 
                ksnet_formatMessage("%7.3f / ", 
                    ip_map_d->stat.triptime_last/1000.0) : strdup(null_str);
            
            // Last 10 max trip time
            char *tcp_triptime_last10_max = ip_map_d != NULL ? 
                ksnet_formatMessage("%.3f ms", 
                    ip_map_d->stat.triptime_last_max/1000.0) : strdup(null_str);
                        
            str = ksnet_sformatMessage(str, "%s"
                "%3d %s%s%s\t %3d   %-15s  %5d   %7s %s  %s%s%s\n",
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

                // ARP Trip time type (ms)
                data->mode < 0 ? "" : "ms",

                // TCP Proxy last trip time type (ms)
                "",
                tcp_last_triptime,
                tcp_triptime_last10_max
                    
                // Rx/Tx
                //"", //(data->idx >= 0 && data->direct_con ? itoa( (&kn->host->peers[data->idx])->incomingDataTotal) : ""),
                //"", // (data->idx >= 0 && data->direct_con ? "/" : ""),
                //"" //(data->idx >= 0 && data->direct_con ? itoa( (&kn->host->peers[data->idx])->outgoingDataTotal) : "")
            );
            free(last_triptime);
            free(tcp_last_triptime);
            free(tcp_triptime_last10_max);
        }
        pblIteratorFree(it);
    }
    str = ksnet_sformatMessage(str, "%s%s", str, div);


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

    ksnet_printf(&((ksnetEvMgrClass*)ka->ke)->ksn_cfg, DISPLAY_M, "%s", str);
    num_line = calculate_lines(str);

    free(str);

    return num_line;
}
