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
    char* namespace; ///< Default namespace
    pblKeyFile_t* k; ///< Opened key file or NULL;
    
} ksnPblKfClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnPblKfClass *ksnPblKfInit(void *ke);
void ksnPblKfDestroy(ksnPblKfClass *kf);

void ksnPblKfNamespaceSet(ksnPblKfClass *kf, const char* namespace);
char *ksnPblKfNamespaceGet(ksnPblKfClass *kf);
void *ksnPblKfGet(ksnPblKfClass *kf, const char *key, size_t *data_len);
int ksnPblKfSet(ksnPblKfClass *kf, const char *key, void *data, size_t data_len);
void *ksnPblKfGetNs(ksnPblKfClass *kf, const char *namespace, const char *key, 
        size_t *data_len);
int ksnPblKfSetNs(ksnPblKfClass *kf, const char *namespace, const char *key, 
        void *data, size_t data_len);

#ifdef	__cplusplus
}
#endif

#endif	/* PBL_KF_H */
