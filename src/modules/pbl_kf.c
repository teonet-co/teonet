/** 
 * File:   pbl_kf.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * PBL KeyFile module
 *
 * Created on August 20, 2015, 4:33 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pbl.h>

#include "pbl_kf.h"
#include "conf.h"

/**
 * Initialize PBL KeyFile module
 * 
 * @param ke Pointer to ksnPblKfClass
 * 
 * @return 
 */
ksnPblKfClass *ksnPblKfInit(void *ke) {
    
    ksnPblKfClass *kf = malloc(sizeof(ksnPblKfClass));
    kf->namespace = NULL;
    kf->k = NULL;
    kf->ke = ke;
    
    return kf;
}

/**
 * Destroy PBL KeyFile module
 * 
 * @param kf Pointer to ksnPblKfClass
 */
inline void ksnPblKfDestroy(ksnPblKfClass *kf) {
    
    if(kf != NULL) {
        ksnPblKfNamespaceSet(kf, NULL);
        free(kf);
    }
}

/**
 * Set default namespace
 * 
 * @param kf Pointer to ksnPblKfClass
 * @param namespace String with namespace
 */
inline void ksnPblKfNamespaceSet(ksnPblKfClass *kf, const char* namespace) {
    
    // Free current namespace and close PBL KeyFile
    if(kf->namespace != NULL) free(kf->namespace);
    if(kf->k != NULL) {
        pblKfClose(kf->k);
        kf->k = NULL;
    }
    
    // Set namespace and open (or create) PBL KeyFile
    if(namespace != NULL) {
        
        kf->namespace = strdup(namespace);
        char *path = (char*) namespace;   
        
        // If file exists
        if(access(path, F_OK ) != -1 ) {
            
            kf->k = pblKfOpen(path, 1, NULL);
        } 
        
        // If file doesn't exist
        else {
            
            kf->k = pblKfCreate(path, NULL);
        }

    }
    else kf->namespace = NULL;
}

/**
 * Get default namespace
 * 
 * @param kf Pointer to ksnPblKfClass
 * 
 * @return Return default namespace or NULL if namespace not set, 
 *         should be free after use
 */
inline char *ksnPblKfNamespaceGet(ksnPblKfClass *kf) {
    
    return kf->namespace != NULL ? strdup(kf->namespace) : NULL;
}

/**
 * Get data by key from default namespace
 * 
 * @param kf Pointer to ksnPblKfClass
 * @param key String with key
 * @param data_len [out] Length of data
 * 
 * @return Pointer to data with data_len length or NULL if not found, 
 *         should be free after use
 */
void *ksnPblKfGet(ksnPblKfClass *kf, const char *key, size_t *data_len) {
    
    void *data = NULL;
    char okey[KSN_BUFFER_SM_SIZE];
    size_t okey_len = KSN_BUFFER_SM_SIZE;
    if(kf->k != NULL) {
        
        long rc = pblKfFind(kf->k, PBLLA, (void*) key, strlen(key) + 1, 
                (void*) okey, &okey_len);
        
        if(rc >= 0) {
            
            *data_len = rc;
            if(rc > 0) { 
                data = malloc(rc);
                pblKfRead(kf->k, data, rc);
            }
            else data = strdup(""); // Return empty string if data length equal to 0
        }       
    }
    
    return data;
}

/**
 * Add (insert or update) data by key to default namespace  
 * 
 * @param kf Pointer to ksnPblKfClass
 * @param key String with key
 * @param data Pointer to data
 * @param data_len Data length
 * 
 * @return 0: call went OK; or an error if != 0
 */
int ksnPblKfSet(ksnPblKfClass *kf, const char *key, void *data, 
        size_t data_len) {
    
    int retval = -1;
    
    if(kf->k != NULL)
        retval = pblKfInsert(kf->k, (void*) key, strlen(key) + 1, data, 
                data_len);
    
    return retval;
}

/**
 * Get data by key from namespace
 * 
 * @param kf Pointer to ksnPblKfClass
 * @param namespace String with namespace
 * @param key String with key
 * @param data_len [out] Data length
 * 
 * @return 
 */
void *ksnPblKfGetNs(ksnPblKfClass *kf, const char *namespace, const char *key, 
        size_t *data_len) {

    char *save_namespace = ksnPblKfNamespaceGet(kf);
    ksnPblKfNamespaceSet(kf, namespace);
    void *data = ksnPblKfGet(kf, key, data_len);
    ksnPblKfNamespaceSet(kf, save_namespace);
    if(save_namespace != NULL) free(save_namespace);
    
    return data;
}

/**
 * Add (insert or update) data by key to namespace  
 * 
 * @param kf Pointer to ksnPblKfClass
 * @param key
 * @param data
 * @param data_len
 * 
 * @return 
 */
int ksnPblKfSetNs(ksnPblKfClass *kf, const char *namespace, const char *key, 
        void *data, size_t data_len) {
    
    char *save_namespace = ksnPblKfNamespaceGet(kf);
    ksnPblKfNamespaceSet(kf, namespace);
    int retval = ksnPblKfSet(kf, key, data, data_len);
    ksnPblKfNamespaceSet(kf, save_namespace);
    if(save_namespace != NULL) free(save_namespace);
    
    return retval;
}
