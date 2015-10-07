/** 
 * File:   crypt.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on July 14, 2015, 4:04 PM
 */

#ifndef CRYPT_H
#define	CRYPT_H

#define BLOCK_SIZE 16
#define KEY_SIZE 32

/**
 * ksnetCrypt Class data
 */
typedef struct ksnCryptClass {

//  MCRYPT td;

  unsigned char iv[BLOCK_SIZE+1];
  unsigned char *key;
  int key_len;
  int blocksize;
  void *ke;

} ksnCryptClass;


#ifdef	__cplusplus
extern "C" {
#endif

ksnCryptClass *ksnCryptInit(void *ke);
void ksnCryptDestroy(ksnCryptClass *kcr);
void *ksnEncryptPackage(ksnCryptClass *kcr, void *package,
                        size_t package_len, void *buffer, size_t *encrypt_len);
void *ksnDecryptPackage(ksnCryptClass *kcr, void* package,
                        size_t package_len, size_t *decrypt_len);
int ksnCheckEncrypted(void *package, size_t package_len);


#ifdef	__cplusplus
}
#endif

#endif	/* CRYPT_H */

