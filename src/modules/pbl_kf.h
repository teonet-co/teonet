/** 
 * File:   pbl_kf.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * PBL key file module
 *
 * Created on August 20, 2015, 4:33 PM
 */

#ifndef PBL_KF_H
#define	PBL_KF_H

/**
 * PBL KeyFile data 
 */
typedef struct ksnPblKfClass {
    
    void *ke; ///< Pointer to the ksnEvMgrClass
    
} ksnPblKfClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnPblKfClass *ksnPblKfInit(void *ke);
void ksnPblKfDestroy(ksnPblKfClass *kf);

#ifdef	__cplusplus
}
#endif

#endif	/* PBL_KF_H */
