/**
 * File:   net_tun.h
 * Author: Kirill Scherba
 *
 * Created on May 10, 2015, 3:27 PM
 *
 * KSNet network tunnel module
 */

#ifndef NET_TUN_H
#define	NET_TUN_H

#include <pbl.h>

/**
 * KSNet network tunnel class data
 */
typedef struct ksnTunClass {

    PblMap* list;   ///< List to store KSNet tunnel servers
    PblMap* map;    ///< Hash Map to store KSNet tunnel table
    void *ke;       ///< Pointer to ksnTermClass

} ksnTunClass;


#ifdef	__cplusplus
extern "C" {
#endif


ksnTunClass *ksnTunInit(void *ke);
void ksnTunDestroy(ksnTunClass *ktun);
void ksnTunCreate(ksnTunClass * ktun, uint16_t port, char *to,
                  uint16_t to_port, char *to_ip);

int cmd_tun_cb(ksnTunClass * ktun, ksnCorePacketData *rd);
char *ksnTunListShow(ksnTunClass * ktun);
void *ksnTunListRemove(ksnTunClass *ktun, int fd);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_TUN_H */
