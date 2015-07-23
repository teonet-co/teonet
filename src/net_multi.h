/** 
 * File:   net_multi.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on July 23, 2015, 11:46 AM
 */

#ifndef NET_MULTI_H
#define	NET_MULTI_H

/**
 * ksnMultiClass module data
 */
typedef struct ksnMultiClass {
    
} ksnMultiClass;


/**
 * ksnMultiClass initialize input data
 */
typedef struct ksnMultiData {
    
} ksnMultiData;

#ifdef	__cplusplus
extern "C" {
#endif

ksnMultiClass *ksnMultiInit(ksnMultiData *md);
void ksnMultiDestroy(ksnMultiClass *km);

#ifdef	__cplusplus
}
#endif

#endif	/* NET_MULTI_H */
