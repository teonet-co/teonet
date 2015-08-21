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
#include <sys/stat.h>

#include <pbl.h>

#include "pbl_kf.h"
#include "conf.h"
#include "utils/utils.h"

/**
 * Initialize PBL KeyFile module
 * 
 * @param ke Pointer to ksnTDBClass
 * 
 * @return 
 */
ksnTDBClass *ksnTDBinit(void *ke) {
    
    ksnTDBClass *kf = malloc(sizeof(ksnTDBClass));
    kf->namespace = NULL;
    kf->k = NULL;
    kf->ke = ke;
    
    return kf;
}

/**
 * Destroy PBL KeyFile module
 * 
 * @param kf Pointer to ksnTDBClass
 */
inline void ksnTDBdestroy(ksnTDBClass *kf) {
    
    if(kf != NULL) {
        ksnTDBnamespaceSet(kf, NULL);
        free(kf);
    }
}

#define get_file_path(namespace) \
        char path[PATH_MAX], *data_path = (char*)getDataPath(); \
        mkdir(data_path, 0755); \
        strncpy(path, data_path, PATH_MAX); \
        strncat(path, "/", PATH_MAX - strlen(path) - 1); \
        strncat(path, namespace, PATH_MAX - strlen(path) - 1)

/**
 * Set current namespace
 * 
 * Set namespace to use in ksnTdbGet, ksnTdbSet and ksnTdbDelete functions 
 * to get, set or delete data without select namespace.
 * 
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 */
void ksnTDBnamespaceSet(ksnTDBClass *kf, const char* namespace) {
    
    // Free current namespace and close PBL KeyFile
    if(kf->namespace != NULL) free(kf->namespace);
    if(kf->k != NULL) {
        pblKfClose(kf->k);
        kf->k = NULL;
    }
    
    // Set namespace and open (or create) PBL KeyFile
    if(namespace != NULL) {
        
        kf->namespace = strdup(namespace);
        
        // File path name
        get_file_path(namespace);
        
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
 * Get current namespace
 * 
 * @param kf Pointer to ksnTDBClass
 * 
 * @return Return default namespace or NULL if namespace not set, 
 *         should be free after use
 */
inline char *ksnTDBnamespaceGet(ksnTDBClass *kf) {
    
    return kf->namespace != NULL ? strdup(kf->namespace) : NULL;
}

/**
 * Remove namespace and all it contains
 * 
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 */
void ksnTDBnamespaceRemove(ksnTDBClass *kf, const char* namespace) {
    
    ksnTDBnamespaceSet(kf, NULL);
    get_file_path(namespace);    
    remove(path); // Remove test file if exist
}

/**
 * Get data by key from default namespace
 * 
 * @param kf Pointer to ksnTDBClass
 * @param key String with key
 * @param data_len [out] Length of data
 * 
 * @return Pointer to data with data_len length or NULL if not found, 
 *         should be free after use
 */
void *ksnTDBget(ksnTDBClass *kf, const char *key, size_t *data_len) {
    
    *data_len = 0;
    void *data = NULL;
    char okey[KSN_BUFFER_SM_SIZE];
    size_t okey_len = KSN_BUFFER_SM_SIZE;
    
    if(kf->k != NULL) {
        
        long rc = pblKfFind(kf->k, /*PBLEQ*/ PBLLA, (void*) key, strlen(key) + 1, 
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
 * @param kf Pointer to ksnTDBClass
 * @param key String with key
 * @param data Pointer to data
 * @param data_len Data length
 * 
 * @return 0: call went OK; or an error if != 0
 */
int ksnTDBset(ksnTDBClass *kf, const char *key, void *data, 
        size_t data_len) {
    
    int retval = -1;
    
    if(kf->k != NULL)
        retval = pblKfInsert(kf->k, (void*) key, strlen(key) + 1, data, 
                data_len);
    
    return retval;
}

/**
 * Delete all records with key
 * 
 * @param Pointer to ksnTDBClass
 * @param key String with key
 * 
 * @return 0: call went OK; != 0 if key not found or some error occurred
 */
int ksnTDBdelete(ksnTDBClass *kf, const char *key) {
    
    int retval = -1;
    
    if(kf->k != NULL) {
        
        void *data;
        size_t data_len;
        while((data=ksnTDBget(kf, key, &data_len)) != NULL) {
            pblKfDelete(kf->k);
            free(data);
            retval = 0;
        }
    }
    
    return retval;
}

/**
 * Get data by key from namespace
 * 
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key String with key
 * @param data_len [out] Data length
 * 
 * @return 
 */
void *ksnTDBgetNs(ksnTDBClass *kf, const char *namespace, const char *key, 
        size_t *data_len) {

    char *save_namespace = ksnTDBnamespaceGet(kf);
    ksnTDBnamespaceSet(kf, namespace);
    void *data = ksnTDBget(kf, key, data_len);
    ksnTDBnamespaceSet(kf, save_namespace);
    if(save_namespace != NULL) free(save_namespace);
    
    return data;
}

/**
 * Add (insert or update) data by key to namespace  
 * 
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key String with key
 * @param data Pointer to data
 * @param data_len [out] Data length
 * 
 * @return 
 */
int ksnTDBsetNs(ksnTDBClass *kf, const char *namespace, const char *key, 
        void *data, size_t data_len) {
    
    char *save_namespace = ksnTDBnamespaceGet(kf);
    ksnTDBnamespaceSet(kf, namespace);
    int retval = ksnTDBset(kf, key, data, data_len);
    ksnTDBnamespaceSet(kf, save_namespace);
    if(save_namespace != NULL) free(save_namespace);
    
    return retval;
}

/**
 * Delete all records with key
 * 
 * @param Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key String with key
 * 
 * @return 0: call went OK; != 0 if key not found or some error occurred
 */
int ksnTDBdeleteNs(ksnTDBClass *kf, const char *namespace, const char *key) {
    
    char *save_namespace = ksnTDBnamespaceGet(kf);
    ksnTDBnamespaceSet(kf, namespace);
    int retval = ksnTDBdelete(kf, key);
    ksnTDBnamespaceSet(kf, save_namespace);
    if(save_namespace != NULL) free(save_namespace);
    
    return retval;
}