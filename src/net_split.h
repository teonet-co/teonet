/** 
 * File:   net_split.h
 * Author: Kirill Scherba
 *
 * Created on July 17, 2015, 10:20 AM
 * 
 * Module to split large packets when send and combine it when receive.
 * 
 */

#ifndef NET_SPLIT_H
#define	NET_SPLIT_H

#include "ev_mgr.h"

/**
 * KSNet split class data
 */
typedef struct ksnSplitClass {
    
    ksnCommandClass *kc;
    
} ksnSplitClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnSplitClass *ksnSplitInit(ksnCommandClass *kc);
void ksnSplitDestroy(ksnSplitClass *ks);


#ifdef	__cplusplus
}
#endif

#endif	/* NET_SPLIT_H */
