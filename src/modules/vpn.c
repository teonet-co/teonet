/**
 * File:   vpn.c
 * Author: Kirill Scherba
 *
 * Created on April 4, 2015, 1:19 AM
 * Update to use with KSMesh on May 3, 2015, 0:19:40
 **
 * KSNet VPN Module
 *
 */

#include "config/config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_MINGW
#define Windows
#endif

#include "ev_mgr.h"
#include "tuntap.h"
#include "utils/utils.h"

/**
 * MAC address structure
 */
typedef struct mac_addr {
    unsigned char d[6];
} mac_addr;

/**
 * The Beginning of Ethernet packet structure
 */
struct ether_head {
    //unsigned char preamble[8];
    //unsigned char delimiter;
    mac_addr destination;
    mac_addr source;
};

/**
 * VPN List data structure
 */
typedef struct vpn_list_data {
    const char *name;
    mac_addr mac;
} vpn_list_data;


// Local functions
int ksnVpnStart(ksnVpnClass *kvpn);
// List
void* map_find_by_name(ksnVpnClass *kvpn, const char* name);
void* map_find_by_mac(ksnVpnClass *kvpn, mac_addr *mac);
// MAC address
int mac_is_broadcast(mac_addr *mac);
char* mac_to_str (mac_addr *mac_addr);
int mac_cmp(mac_addr *dst, mac_addr *src);
mac_addr *mac_copy (mac_addr *dst, mac_addr *src);
// Peers
/**
 * Send packet to peer
 *
 * @param peer_name
 * @param data
 * @param data_len
 * @return Always return 0
 */
#define send_to_peer(kvpn, peer_name,data,data_len) \
    ksnCoreSendCmdto(((ksnetEvMgrClass*)(kvpn->ke))->kc, peer_name, CMD_VPN, data, data_len)
void send_to_all(ksnVpnClass *kvpn, void *data, size_t data_len);

#define KSNET_VPN_DEFAULT_ALLOW 1
#define DEBUG_THIS DEBUG //MESSAGE  // Debug type
#define SHOW_DEBUG 0    // Show debug in critical sections
#define KSN_VPN_USE_HASH_MAP


/**
 * Initialize VPN module
 */
//ksnVpnClass 
void* ksnVpnInit(void *ke) {

    if(!((ksnetEvMgrClass*)ke)->ksn_cfg.vpn_connect_f) return NULL;

    ksnVpnClass *kvpn = malloc(sizeof(ksnVpnClass));

    //kvpn->ksnet_vpn_allow = KSNET_VPN_DEFAULT_ALLOW;
    kvpn->ksnet_vpn_started = 0;
    kvpn->ksnet_vpn_map = NULL;        ///< MAC Hash Map
    kvpn->ksn_tap_dev = NULL;          ///< TUNTAP Device
    kvpn->tuntap_name = NULL;          ///< TUNTAP Device name

    ((ksnetEvMgrClass*)ke)->kvpn = kvpn;
    kvpn->ke = ke;

    // Create VPN Hash Map
    kvpn->ksnet_vpn_map = pblMapNewHashMap();

    #ifdef DEBUG_KSNET
    ksnet_printf(&((ksnetEvMgrClass*)ke)->ksn_cfg, DEBUG_VV, 
        "VPN module have been initialized\n");
    #endif

    //usleep(500000);
    kvpn->ksnet_vpn_started = !ksnVpnStart(kvpn);
    //usleep(250000);

    return kvpn;
}

/**
 * De-initialize Event manager module
 */
void ksnVpnDestroy(void *vpn) {

    ksnVpnClass *kvpn = vpn;
    
    if(kvpn != NULL) {

        ksnetEvMgrClass *ke = kvpn->ke;
            
        if(kvpn->ksn_tap_dev != NULL) {
            tuntap_destroy(kvpn->ksn_tap_dev);
            kvpn->ksn_tap_dev = NULL;
        }
        pblMapFree(kvpn->ksnet_vpn_map);
        free(kvpn);
        ke->kvpn = NULL;
        
        #ifdef DEBUG_KSNET
        ksnet_printf(&((ksnetEvMgrClass*)ke)->ksn_cfg, DEBUG_VV, 
            "VPN module have been de-initialized\n");
        #endif
    }
}

/**
 * Get VPN commands from peers and process it (resend to TUNTAP interface
 *
 * @param cmd
 * @return
 */
int cmd_vpn_cb(ksnVpnClass *kvpn, char *from, void *data, size_t data_len) {

    if(kvpn == NULL || !kvpn->ksnet_vpn_started) return 1;

    // Ethernet packet header
    struct ether_head *eth = data;

    #if SHOW_DEBUG
    char *destination = mac_to_str(&eth->destination);
    char *source = mac_to_str(&eth->source);
    #endif

    // Show VPN Command info
    #if SHOW_DEBUG
    #ifdef DEBUG_KSNET
    ksnet_printf(
        &((ksnetEvMgrClass*)kvpn->ke)->ksn_cfg,
        DEBUG_THIS,
        "Command VPN received from %s peer, destination: %s , source: %s \n",
        from, destination, source
    );
    #endif
    #endif

    int retval = 1;
    mac_addr *mac;
    // Check source MAC address in VPN List and add it if absent
    if( (map_find_by_mac(kvpn, &eth->source)) == NULL ) {

        // Check name in VPN List and remove if already present (MAC Changed)
        if((mac = map_find_by_name(kvpn, from)) != NULL) {
            size_t val_len;
            pblMapRemove(kvpn->ksnet_vpn_map, mac, sizeof(mac_addr), &val_len);
        }

        // Insert name and MAC to list
        #if SHOW_DEBUG
        #ifdef DEBUG_KSNET
        ksnet_printf(
                &((ksnetEvMgrClass*)kvpn->ke)->ksn_cfg,
                DEBUG_THIS, "insert source %s to VPN list\n", source);
        #endif
        #endif

        pblMapAdd(kvpn->ksnet_vpn_map,
                  &eth->source, sizeof(mac_addr),   // MAC Address
                  from, strlen(from) + 1  // Peer name
                );

        #ifdef DEBUG_KSNET
        if(((ksnetEvMgrClass*)kvpn->ke)->ksn_cfg.show_debug_vv_f)
            ksnVpnListShow(kvpn);
        #endif
    }

    // Send packet to interface
    write(kvpn->tuntap_fd, data, data_len);

    // Free memory
    #if SHOW_DEBUG
    free(destination);
    free(source);
    #endif

    return retval;
}

/**
 * Broadcast data structure
 */
struct packet_data {

    void *data;
    size_t data_len;
};

/**
 * Send to one peer of broadcast request
 *
 * @param ka
 * @param peer_name
 * @param arp
 * @param pd
 *
 * @return
 */
int send_to_one_cb(ksnetArpClass *ka, char *peer_name, ksnet_arp_data *arp,
                   void *pd) {

    send_to_peer(((ksnetEvMgrClass*)(ka->ke))->kvpn,
                 peer_name,
                 ((struct packet_data *)pd)->data,
                 ((struct packet_data *)pd)->data_len);

    return 0;
}

/**
 * Send broadcast packet to all ksnet peers
 *
 * @param data
 * @param data_len
 */
void send_to_all(ksnVpnClass *kvpn, void *data, size_t data_len) {

    struct packet_data pd;
    pd.data = data;
    pd.data_len = data_len;
    ksnetArpGetAll(((ksnetEvMgrClass*)kvpn->ke)->kc->ka, send_to_one_cb, &pd);
}

/**
 * TUNTAP IO Callback
 *
 * @param loop
 * @param w
 * @param revents
 */
static void tuntap_io_cb (EV_P_ ev_io *w, int revents) {

    // Note that "buffer" should be at least the MTU size of the interface, eg 1500 bytes
    unsigned char buffer[KSN_BUFFER_DB_SIZE];

    // Get pointer to VPN Class
    ksnVpnClass *kvpn = w->data;

    // Read data from device
    int nread = read(w->fd, buffer, sizeof(buffer));

    if(nread < 0) {
        perror("Reading from interface");
        close(w->fd);
        //exit(1);
        return;
    }

    void *name;
    struct ether_head *eth = (struct ether_head *) buffer;
    #if SHOW_DEBUG
    char *destination = mac_to_str(&eth->destination);
    char *source = mac_to_str(&eth->source);
    #endif

    // Show statistic message
    #if SHOW_DEBUG
    #ifdef DEBUG_KSNET
    ksnet_printf(
            &((ksnetEvMgrClass*)kvpn->ke)->ksn_cfg,
            DEBUG_THIS,
            "Read %d bytes from interface %s, \tdestination: %s \tsource %s\n",
            nread, kvpn->tuntap_name, destination, source);
    #endif
    #endif

    // If destination MAC address is Broadcast then send packet to all ksnet peers
    if(mac_is_broadcast(&eth->destination)) {
        #if SHOW_DEBUG
        #ifdef DEBUG_KSNET
        ksnet_printf(
                &((ksnetEvMgrClass*)kvpn->ke)->ksn_cfg,
                DEBUG_THIS, "send to all...\n");
        #endif
        #endif
        send_to_all(kvpn, buffer, nread);
    }
    // Send request to known peer
    else if( (name = map_find_by_mac(kvpn, &eth->destination)) != NULL ) {
        send_to_peer(kvpn, name, buffer, nread);
    }

    // Not defined MAC (or special command)
    else {
        #if SHOW_DEBUG
        #ifdef DEBUG_KSNET
        ksnet_printf(
                &((ksnetEvMgrClass*)kvpn->ke)->ksn_cfg,
                DEBUG_THIS, "unknown MAC %s\n", destination);
        #endif
        #endif
    }

    // Free memory
    #if SHOW_DEBUG
    free(destination);
    free(source);
    #endif
}

/**
 * Open TAP interface, set IP to ifconfig and connect interface to ksnet VPN
 */
int ksnVpnStart(ksnVpnClass *kvpn) {

    int retval = 0;

    kvpn->ksn_tap_dev = tuntap_init();

    if(tuntap_start(kvpn->ksn_tap_dev,
                    TUNTAP_MODE_ETHERNET /*TUNTAP_MODE_TUNNEL*/,
                    TUNTAP_ID_ANY) == -1) {

        retval = 1;
    }

    if(!retval) {

        kvpn->tuntap_fd = TUNTAP_GET_FD(kvpn->ksn_tap_dev);
        kvpn->tuntap_name = tuntap_get_ifname(kvpn->ksn_tap_dev);
        char *tuntap_haddr = tuntap_get_hwaddr(kvpn->ksn_tap_dev);

        // Set interface name
        ksnetEvMgrClass *ke = kvpn->ke;

        if(ke->ksn_cfg.vpn_dev_name[0] != '\0') {
            size_t len = strlen(ke->ksn_cfg.vpn_dev_name);
            char *name = isdigit(ke->ksn_cfg.vpn_dev_name[len-1]) ?
                strdup(ke->ksn_cfg.vpn_dev_name) :
                ksnet_formatMessage("%s0", ke->ksn_cfg.vpn_dev_name);
            tuntap_set_ifname(kvpn->ksn_tap_dev, name);
            free(name);
            kvpn->tuntap_name = tuntap_get_ifname(kvpn->ksn_tap_dev);
        }

        // Set interface Hardware address
        if(ke->ksn_cfg.vpn_dev_hwaddr[0] != '\0') {
            tuntap_set_hwaddr(kvpn->ksn_tap_dev, ke->ksn_cfg.vpn_dev_hwaddr);
            tuntap_haddr = tuntap_get_hwaddr(kvpn->ksn_tap_dev);
        } else {
            ksnet_addHWAddrConfig(&ke->ksn_cfg, tuntap_haddr);
        }

        // Show success message
        ksnet_printf(&ke->ksn_cfg, MESSAGE,
                     "Interface %s (addr: %s) opened ...\n",
                     kvpn->tuntap_name, tuntap_haddr);

        // Interface Up
        if (tuntap_up(kvpn->ksn_tap_dev) == -1) {
//		ret = 1;
//		goto clean;
        } else {

            // Set IP and mask
            if(tuntap_set_ip(kvpn->ksn_tap_dev, ke->ksn_cfg.vpn_ip,
                             ke->ksn_cfg.vpn_ip_net) == -1) {
    //		ret = 1;
            }
            else {
                ksnet_printf(&ke->ksn_cfg, MESSAGE,
                             "VPN IP set to: %s/%d\n\n",
                             ke->ksn_cfg.vpn_ip, ke->ksn_cfg.vpn_ip_net);

                // Execute if-up.sh script
                char *buffer = ksnet_formatMessage(
                    "%s/if-up.sh %s", getDataPath(), kvpn->tuntap_name);
                system(buffer);
                free(buffer);
            }
        }
//clean:
        // Switch to non-block mode

        // Create TUNTAP Get data event callback
        ev_io *tuntap_io = (struct ev_io*) malloc (sizeof(struct ev_io));
        ev_io_init (tuntap_io, tuntap_io_cb, kvpn->tuntap_fd, EV_READ);
        tuntap_io->data = kvpn;
        ev_io_start (EV_DEFAULT_ tuntap_io);
    }

    return retval;
}

/**
 * Mac address structure to c string convert
 *
 * @param mac_addr Null terminated string with MAC address, should be free
 *                 after use
 * @return
 */
char* mac_to_str (mac_addr *mac_addr) {

    return ksnet_formatMessage("%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr->d[0],
            mac_addr->d[1],
            mac_addr->d[2],
            mac_addr->d[3],
            mac_addr->d[4],
            mac_addr->d[5] );
}

/**
 * Copy MAC Address
 *
 * @param dst
 * @param src
 * @return
 */
mac_addr* mac_copy(mac_addr *dst, mac_addr *src) {

    memcpy((void*)dst->d, (void*)src->d, sizeof(mac_addr));

    return dst;
}

/**
 * Check if MAC address is Broadcast or Multicast
 *
 * @param mac
 * @return Return true if MAC address is broadcast
 */
int mac_is_broadcast(mac_addr *mac) {

    int i, retval = 1;

//    // Check Multicast
//    unsigned char d[6] = { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB };
//    mac_addr *m = (mac_addr*) d;
//    if(mac_cmp(mac, m )) return 1;

    // Check Broadcast
    for(i=0; i < 6; i++) {
        if(mac->d[i] != 0xff) {
            retval = 0;
            break;
        }
    }

    return retval;
}

/**
 * Compare two MAC address
 *
 * @param dst
 * @param src
 *
 * @return Return true if MAC address is quale
 */
int mac_cmp(mac_addr *dst, mac_addr *src) {

    int i, retval = 1;

    for(i=0; i < 6; i++) {

        if(dst->d[i] != src->d[i]) {
            retval = 0;
            break;
        }
    }

    return retval;
}

/**
 * Show VPN list
 */
void ksnVpnListShow(ksnVpnClass *kvpn) {

    if(kvpn == NULL) {
        printf("The VPN switch off\n");
        return;
    }

    PblIterator *it =  pblMapIteratorNew(kvpn->ksnet_vpn_map);
    printf("Number of peers in VPN: %d\n", pblMapSize(kvpn->ksnet_vpn_map));
    if(it != NULL) {
        while(pblIteratorHasNext(it)) {
            void *entry = pblIteratorNext(it);
            const char *mac_str = mac_to_str(pblMapEntryKey(entry));
            printf("name: %s, mac: %s\n", (char*) pblMapEntryValue(entry), mac_str);
            free((void*) mac_str);
        }
        pblIteratorFree(it);
    }
}

/**
 * Find MAC by name
 *
 * @param name Name of peer
 * @return Return pointer to MAC or NULL if name not found in map
 */
void* map_find_by_name(ksnVpnClass *kvpn, const char* name) {

    char *mac = NULL;
    PblIterator *it =  pblMapIteratorNew(kvpn->ksnet_vpn_map);
    while(pblIteratorHasNext(it)) {

        void *entry = pblIteratorNext(it);
        if(!strcmp(pblMapEntryValue(entry), name)) {
            mac = pblMapEntryKey(entry);
            break;
        }
    }
    pblIteratorFree(it);

    return mac;
}

/**
 * Find name by MAC Address
 *
 * @param mac MAC Address
 * @return Return pointer to name or NULL if MAC Address not found in map
 */
void *map_find_by_mac(ksnVpnClass *kvpn, mac_addr *mac) {

    size_t val_len;
    return pblMapGet(kvpn->ksnet_vpn_map, mac, sizeof(mac_addr), &val_len);
}
