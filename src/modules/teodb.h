/** 
 * File:   teodb.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet database module based at PBL KeyFile
 *
 * Created on August 20, 2015, 4:33 PM
 */

#ifndef TEODB_H
#define	TEODB_H

#include "utils/string_arr.h"

/**
 * PBL KeyFile data 
 */
typedef struct ksnTDBClass {
    
    void *ke; ///< Pointer to the ksnEvMgrClass
    char* namespace; ///< Default namespace
    pblKeyFile_t* k; ///< Opened key file or NULL;
    
} ksnTDBClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnTDBClass *ksnTDBinit(void *ke);
void ksnTDBdestroy(ksnTDBClass *kf);

void ksnTDBnamespaceSet(ksnTDBClass *kf, const char* namespace);
char *ksnTDBnamespaceGet(ksnTDBClass *kf);
void ksnTDBnamespaceRemove(ksnTDBClass *kf, const char* namespace);

void *ksnTDBgetStr(ksnTDBClass *kf, const char *key, size_t *data_len);
void *ksnTDBget(ksnTDBClass *kf, const void *key, size_t key_len, 
        size_t *data_len);
int ksnTDBsetStr(ksnTDBClass *kf, const char *key, void *data, size_t data_len);
int ksnTDBset(ksnTDBClass *kf, const void *key, size_t key_len, void *data,
        size_t data_len);
int ksnTDBdeleteStr(ksnTDBClass *kf, const char *key);
int ksnTDBdelete(ksnTDBClass *kf, const void *key, size_t key_len);

void *ksnTDBgetNsStr(ksnTDBClass *kf, const char *namespace, const char *key, 
        size_t *data_len);
void *ksnTDBgetNs(ksnTDBClass *kf, const char *namespace, const void *key, 
        size_t key_len, size_t *data_len);
int ksnTDBsetNsStr(ksnTDBClass *kf, const char *namespace, const char *key, 
        void *data, size_t data_len);
int ksnTDBsetNs(ksnTDBClass *kf, const char *namespace, const void *key, 
        size_t key_len, void *data, size_t data_len);
int ksnTDBdeleteNsStr(ksnTDBClass *kf, const char *namespace, const char *key);
int ksnTDBdeleteNs(ksnTDBClass *kf, const char *namespace, const void *key, 
        size_t key_len);
int ksnTDBkeyList(ksnTDBClass *kf, ksnet_stringArr *argv);

#ifdef	__cplusplus
}
#endif

#endif	/* TEODB_H */
