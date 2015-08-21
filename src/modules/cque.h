/** 
 * File:   cque.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on August 21, 2015, 12:10 PM
 */

#ifndef CQUE_H
#define	CQUE_H

typedef struct ksnCQue {
    
    void *ke; ///< Pointer to ksnEvMgrClass
    
} ksnCQue;

#ifdef	__cplusplus
extern "C" {
#endif

ksnCQue *ksnCQueInit(void *ke);
void ksnCQueDestroy(ksnCQue *kq);


#ifdef	__cplusplus
}
#endif

#endif	/* CQUE_H */
