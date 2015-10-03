/** 
 * File:   stream.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet stream module
 *
 * Created on October 3, 2015, 3:02 PM
 */

#ifndef STREAM_H
#define	STREAM_H

typedef struct ksnStreamClass {
    
    void *ke;       ///< Pointer to ksnetEvMgrClass
    
} ksnStreamClass;

#ifdef	__cplusplus
extern "C" {
#endif

ksnStreamClass *ksnStreamInit(void *ke);
void ksnStreamDestroy(ksnStreamClass *ks);


#ifdef	__cplusplus
}
#endif

#endif	/* STREAM_H */

