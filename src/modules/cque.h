/* 
 * File:   cque.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on August 21, 2015, 12:10 PM
 */

#ifndef CQUE_H
#define	CQUE_H

#include <ev.h>
#include <pbl.h>
#include <stdint.h>

/**
 * ksnCQue Class structure definition
 */
typedef struct ksnCQueClass {
    
    void *ke; ///< Pointer to ksnEvMgrClass
    uint32_t id; ///< New callback queue ID
    PblMap *cque_map; ///< Pointer to the callback queue pblMap
    uint8_t event_f; ///< Send cque event if true
    
} ksnCQueClass;

/**
 * ksnCQue callback function definition 
 * 
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback 
 * @param data User data selected in ksnCQueAdd() function
 */
typedef void (*ksnCQueCallback) (uint32_t id, int type, void *data);

/**
 * ksnCQue data structure
 */
typedef struct ksnCQueData {
    
    ksnCQueCallback cb; ///< Pointer to callback function
    ksnCQueClass *kq; ///< Pointer to ksnCQueClass
    double timeout; ///< Timeout value
    uint32_t id; ///< Callback ID (equal to key)
    //char data[]; ///< \todo: Some data 
    void *data; ///< User data
    ev_timer w; ///< Timeout watcher
    
} ksnCQueData;

/**
 * Find data in CQueue callback function
 */
typedef int (*ksnCQueCompare) (void *find, void *data);


#ifdef	__cplusplus
extern "C" {
#endif

ksnCQueClass *ksnCQueInit(void *ke, uint8_t send_event);
void ksnCQueDestroy(ksnCQueClass *kq);

int ksnCQueExec(ksnCQueClass *kq, uint32_t id);
void * ksnCQueGetData(ksnCQueClass *kq, uint32_t id);
int ksnCQueSetData(ksnCQueClass *kq, uint32_t id, void *data);
ksnCQueData *ksnCQueAdd(ksnCQueClass *kq, ksnCQueCallback cb, double timeout, 
        void *data);
int ksnCQueRemove(ksnCQueClass *kq, uint32_t id);
void *ksnCQueFindData(ksnCQueClass *kq, void* find, ksnCQueCompare compare, size_t *key_length);

void *pblMapRemoveFree(PblMap * map, void * key, size_t keyLength, 
        size_t * valueLengthPtr );

#ifdef	__cplusplus
}
#endif

#endif	/* CQUE_H */
