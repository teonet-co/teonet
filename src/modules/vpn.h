/**
 * File:   vpn.h
 * Author: Kirill Scherba
 *
 * Created on April 4, 2015, 1:19 AM
 * Update to use with KSMesh on May 3, 2015, 0:19:40
 **
 * KSNet VPN Module
 */

#ifndef VPN_H
#define	VPN_H

#include <stdio.h>
#include <pbl.h>

#include "ev_mgr.h"

/**
 * KSNet VPN Class data
 */
typedef struct ksnVpnClass {

    int ksnet_vpn_started;          ///< VPN Started
    PblMap *ksnet_vpn_map;          ///< MAC Hash Map
    struct device *ksn_tap_dev;     ///< TUNTAP Device
    char *tuntap_name;              ///< TUNTAP Device name
    int tuntap_fd;                  ///< TUNTAP Device FD
    void *ke;                       ///< Pointer to Event manager class object

} ksnVpnClass;

#ifdef	__cplusplus
extern "C" {
#endif

//ksnVpnClass 
void *ksnVpnInit(void *ke);
void ksnVpnDestroy(void *vpn);

int cmd_vpn_cb(ksnVpnClass *kvpn, char *from, void *data, size_t data_len);
void ksnVpnListShow(ksnVpnClass *kvpn);

#ifdef	__cplusplus
}
#endif

#endif	/* VPN_H */
