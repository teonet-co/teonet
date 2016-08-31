/**
 * \file   modules/teodb.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet database module based at PBL KeyFile
 *
 * Created on August 20, 2015, 4:33 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "config.h"

#ifdef HAVE_DARWIN
#include <sys/syslimits.h>
#endif

#include <pbl.h>

#include "teodb.h"
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
    kf->defNameSpace = NULL;
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
    if(kf->defNameSpace != NULL) free(kf->defNameSpace);
    if(kf->k != NULL) {
        pblKfClose(kf->k);
        kf->k = NULL;
    }

    // Set namespace and open (or create) PBL KeyFile
    if(namespace != NULL) {

        kf->defNameSpace = strdup(namespace);

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
    else kf->defNameSpace = NULL;
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

    return kf->defNameSpace != NULL ? strdup(kf->defNameSpace) : NULL;
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
 * Flush a key file
 * 
 * @param kf
 * @return 
 */
inline int ksnTDBflush(ksnTDBClass *kf) {
    return pblKfFlush(kf->k);
}

/**
 * Get data by string key from default namespace
 *
 * @param kf Pointer to ksnTDBClass
 * @param key String with key
 * @param data_len [out] Length of data
 *
 * @return Pointer to data with data_len length or NULL if not found,
 *         should be free after use
 */
inline void *ksnTDBgetStr(ksnTDBClass *kf, const char *key, size_t *data_len) {

    return ksnTDBget(kf, key, strlen(key) + 1, data_len);
}

/**
 * Get data by string key from default namespace
 *
 * @param kf Pointer to ksnTDBClass
 * @param key String with key
 * @param key_len Key length
 * @param data_len [out] Length of data
 *
 * @return Pointer to data with data_len length or NULL if not found,
 *         should be free after use
 */
void *ksnTDBget(ksnTDBClass *kf, const void *key, size_t key_len, 
        size_t *data_len) {

    *data_len = 0;
    void *data = NULL;
    char okey[KSN_BUFFER_SM_SIZE];
    size_t okey_len = KSN_BUFFER_SM_SIZE;

    if(kf->k != NULL) {

        long rc = pblKfFind(kf->k, PBLLA, (void*) key, key_len,
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
 * Add (insert or update) data by string key to default namespace
 *
 * @param kf Pointer to ksnTDBClass
 * @param key String with key
 * @param data Pointer to data
 * @param data_len Data length
 *
 * @return 0: call went OK; or an error if != 0
 */
inline int ksnTDBsetStr(ksnTDBClass *kf, const char *key, void *data,
        size_t data_len) {

    return ksnTDBset(kf, key, strlen(key) + 1, data, data_len);
}

/**
 * Add (insert or update) data by key to default namespace
 *
 * @param kf Pointer to ksnTDBClass
 * @param key Binary key
 * @param key_len Key length
 * @param data Pointer to data
 * @param data_len Data length
 *
 * @return 0: call went OK; or an error if != 0
 */
int ksnTDBset(ksnTDBClass *kf, const void *key, size_t key_len, void *data,
        size_t data_len) {

    int retval = -1;

    if(kf->k != NULL) {              
        // \todo Check key & data, and may be do delete record if data == NULL 
        
        if(key != NULL && data != NULL) {
            
            char okey[KSN_BUFFER_SM_SIZE];
            size_t okey_len = KSN_BUFFER_SM_SIZE;

            // Check if record exists
            long rc = pblKfFind(kf->k, PBLLA, (void*) key, key_len, 
                    (void*) okey, &okey_len);
            
            // Update or insert record
            if(rc >= 0) retval = pblKfUpdate(kf->k, data, data_len);
            else retval = pblKfInsert(kf->k, (void*) key, key_len, data, data_len);
        }
    }

    return retval;
}

/**
 * Delete all records with string key
 *
 * @param kf Pointer to ksnTDBClass
 * @param key String with key
 *
 * @return 0: call went OK; != 0 if key not found or some error occurred
 */
inline int ksnTDBdeleteStr(ksnTDBClass *kf, const char *key) {

    return ksnTDBdelete(kf, key, strlen(key) + 1);
}

/**
 * Delete all records with key
 *
 * @param kf Pointer to ksnTDBClass
 * @param key Binary key
 * @param key_len Key length
 *
 * @return 0: call went OK; != 0 if key not found or some error occurred
 */
int ksnTDBdelete(ksnTDBClass *kf, const void *key, size_t key_len) {

    int retval = -1;

    if(kf->k != NULL) {

        void *data;
        size_t data_len;
        while((data=ksnTDBget(kf, key, key_len, &data_len)) != NULL) {
            pblKfDelete(kf->k);
            free(data);
            retval = 0;
        }
    }

    return retval;
}

/**
 * Get data by string key from namespace
 *
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key String with key
 * @param data_len [out] Data length
 *
 * @return
 */
inline void *ksnTDBgetNsStr(ksnTDBClass *kf, const char *namespace, const char *key,
        size_t *data_len) {

    return ksnTDBgetNs(kf, namespace, key, strlen(key) + 1, data_len);
}

/**
 * Get data by key from namespace
 *
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key Binary key
 * @param key_len Key length
 * @param data_len [out] Data length
 *
 * @return
 */
void *ksnTDBgetNs(ksnTDBClass *kf, const char *namespace, const void *key, 
        size_t key_len, size_t *data_len) {

    char *save_namespace = ksnTDBnamespaceGet(kf);
    ksnTDBnamespaceSet(kf, namespace);
    void *data = ksnTDBget(kf, key, key_len, data_len);
    ksnTDBnamespaceSet(kf, save_namespace);
    if(save_namespace != NULL) free(save_namespace);

    return data;
}

/**
 * Add (insert or update) data by string key to namespace
 *
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key String with key
 * @param data Pointer to data
 * @param data_len [out] Data length
 *
 * @return
 */
inline int ksnTDBsetNsStr(ksnTDBClass *kf, const char *namespace, const char *key,
        void *data, size_t data_len) {

    return ksnTDBsetNs(kf, namespace, key, strlen(key) + 1, data, data_len);
}

/**
 * Add (insert or update) data by key to namespace
 *
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key Binary key
 * @param key_len Key length
 * @param data Pointer to data
 * @param data_len [out] Data length
 *
 * @return
 */
int ksnTDBsetNs(ksnTDBClass *kf, const char *namespace, const void *key, 
        size_t key_len, void *data, size_t data_len) {

    char *save_namespace = ksnTDBnamespaceGet(kf);
    ksnTDBnamespaceSet(kf, namespace);
    int retval = ksnTDBset(kf, key, key_len, data, data_len);
    ksnTDBnamespaceSet(kf, save_namespace);
    if(save_namespace != NULL) free(save_namespace);

    return retval;
}

/**
 * Delete all records with key
 *
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key String with key
 *
 * @return 0: call went OK; != 0 if key not found or some error occurred
 */
inline int ksnTDBdeleteNsStr(ksnTDBClass *kf, const char *namespace, const char *key) {

    return ksnTDBdeleteNs(kf, namespace, key, strlen(key) + 1);
}

/**
 * Delete all records with key
 *
 * @param kf Pointer to ksnTDBClass
 * @param namespace String with namespace
 * @param key Binary key
 * @param key_len Key length
 *
 * @return 0: call went OK; != 0 if key not found or some error occurred
 */
int ksnTDBdeleteNs(ksnTDBClass *kf, const char *namespace, const void *key, 
        size_t key_len) {

    char *save_namespace = ksnTDBnamespaceGet(kf);
    ksnTDBnamespaceSet(kf, namespace);
    int retval = ksnTDBdelete(kf, key, key_len);
    ksnTDBnamespaceSet(kf, save_namespace);
    if(save_namespace != NULL) free(save_namespace);

    return retval;
}

/**
 * Get list of keys // \todo Add parameters: From - from key number, To - to key number
 * 
 * @param kf Pointer to ksnTDBClass
 * @param key
 * @param argv Pointer to ksnet_stringArr
 * 
 * @return Number of 
 */
int ksnTDBkeyList(ksnTDBClass *kf, const char *key, ksnet_stringArr *argv) {
    
    int num_of_key = 0;
    *argv = NULL;
    
    if(kf->k != NULL) {
        
        char okey[KSN_BUFFER_SM_SIZE];
        size_t okey_len = KSN_BUFFER_SM_SIZE;        
                
        long data_len; 
        
        // No key present
        if(key == NULL || key[0] == 0) {
            
            // Get first key
            if((data_len = pblKfGetAbs(kf->k, 0, (void*) okey, &okey_len)) >= 0) {

                // Add first key to string array
                ksnet_stringArrAdd(argv, okey);
                num_of_key++;

                // Get next keys and add it to string array
                while((data_len = pblKfNext(kf->k, okey, &okey_len)) >= 0) {

                    ksnet_stringArrAdd(argv, okey);
                    num_of_key++;
                }
            }
        }
        
        //Key is present
        else {
            
            // Get first key
            size_t key_len = strlen(key) + 1;
            if((data_len = pblKfFind (kf->k, PBLGE, (void *)key, key_len, (void *)okey, 
                    &okey_len )) >= 0) {
                
                if(!strncmp(key, okey, key_len - 1)) {
                    
                    // Add first key to string array
                    ksnet_stringArrAdd(argv, okey);
                    num_of_key++;

                    // Get next keys and add it to string array
                    while((data_len = pblKfNext(kf->k, okey, &okey_len)) >= 0) {

                        if(strncmp(key, okey, key_len - 1)) break;

                        ksnet_stringArrAdd(argv, okey);
                        num_of_key++;
                    }
                }
            }
        }                
    }
    
    return num_of_key;
}
