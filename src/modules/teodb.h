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

void *ksnTDBget(ksnTDBClass *kf, const char *key, size_t *data_len);
int ksnTDBset(ksnTDBClass *kf, const char *key, void *data, size_t data_len);
int ksnTDBdelete(ksnTDBClass *kf, const char *key);

void *ksnTDBgetNs(ksnTDBClass *kf, const char *namespace, const char *key, 
        size_t *data_len);
int ksnTDBsetNs(ksnTDBClass *kf, const char *namespace, const char *key, 
        void *data, size_t data_len);
int ksnTDBdeleteNs(ksnTDBClass *kf, const char *namespace, const char *key);

#ifdef	__cplusplus
}
#endif

#endif	/* TEODB_H */
