/** 
 * File:   net_multi.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on July 23, 2015, 11:46 AM
 */

#ifndef NET_MULTI_H
#define	NET_MULTI_H

#include "ev_mgr.h"

/**
 * ksnMultiClass module data
 */
typedef struct ksnMultiClass {
    
    PblList* list; ///< Pointer to network list
    size_t num; ///< Number of networks
    
} ksnMultiClass;


/**
 * ksnMultiClass initialize input data
 */
typedef struct ksnMultiData {

    int argc; ///< Applications argc
    char** argv; ///< Applications argv
    void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data); ///< Event callback function
    
    size_t num; ///< Number of networks
    const int *ports; ///< Ports arrays
    const char **names; ///< Names arrays
    const char **networks; ///< Networks arrays
    
    int run; ///< Run inside init
    
} ksnMultiData;

#ifdef	__cplusplus
extern "C" {
#endif

ksnMultiClass *ksnMultiInit(ksnMultiData *md);
void ksnMultiDestroy(ksnMultiClass *km);
ksnetEvMgrClass *ksnMultiGet(ksnMultiClass *km, int num);
char *ksnMultiShowListStr(ksnMultiClass *km);
ksnet_arp_data *ksnMultiSendCmdTo(ksnMultiClass *km, char *to, uint8_t cmd, void *data, 
        size_t data_len);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_MULTI_H */
