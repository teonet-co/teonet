/** 
 * File:   crypt.h
 * Author: Kirill Scherba
 *
 * Created on July 14, 2015, 4:04 PM
 */

#ifndef CRYPT_H
#define	CRYPT_H

#define BLOCK_SIZE 16

/**
 * ksnetCrypt Class data
 */
typedef struct ksnCryptClass {

//  MCRYPT td;

  char iv[BLOCK_SIZE+1];
  char *key;
  int key_len;
  int blocksize;

} ksnCryptClass;


#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* CRYPT_H */

