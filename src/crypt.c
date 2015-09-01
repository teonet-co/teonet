/**
 * File:   crypt.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on July 14, 2015, 4:04 PM
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "crypt.h"
#include "ev_mgr.h"
#include "utils/rlutil.h"

// Number of modules started
int num_crypt_module = 0;

/**
 * Module initialize
 *
 * @return Pointer to created ksnCryptClass
 */
ksnCryptClass *ksnCryptInit(void *ke) {

    ksnCryptClass *kcr = malloc(sizeof(ksnCryptClass));
    kcr->ke = ke;

    // A 128 bit IV
    const char *iv = "0123456789012345";
    strncpy((char*)kcr->iv, iv, BLOCK_SIZE);

    static const char *key = "01234567890123456789012345678901";
    kcr->key = (unsigned char*) key;
    kcr->key_len = strlen(key); // 32 - 256 bits
    kcr->blocksize = BLOCK_SIZE;

    // Initialize the library
    if(!num_crypt_module) {
        ERR_load_crypto_strings();
        OpenSSL_add_all_algorithms();
        OPENSSL_config(NULL);
    }
    num_crypt_module++;

    return kcr;
}

/**
 * Module initialize
 *
 * @param kcr
 */
void ksnCryptDestroy(ksnCryptClass *kcr) {

    // Clean up
    if(num_crypt_module == 1) {
        EVP_cleanup();
        ERR_free_strings();
    }
    num_crypt_module--;

    free(kcr);
}

void handleErrors(void) {

  ERR_print_errors_fp(stderr);
  //abort();
}

size_t _encrypt(unsigned char *plaintext, size_t plaintext_len,
        unsigned char *key, unsigned char *iv, void *ciphertext) {

  EVP_CIPHER_CTX *ctx;

  int len;

  int ciphertext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) {
      handleErrors();
      return 0;
  }

  /* Initialise the encryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
      
    handleErrors();
    return 0;
  }

  /* Provide the message to be encrypted, and obtain the encrypted output.
   * EVP_EncryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
      
    handleErrors();
    return 0;
  }
  ciphertext_len = len;

  /* Finalise the encryption. Further ciphertext bytes may be written at
   * this stage.
   */
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
      
      handleErrors();
      return 0;
  }
  ciphertext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return ciphertext_len;
}

/**
 * Decrypt buffer
 *
 * @param ciphertext Encrypted data
 * @param ciphertext_len Encrypted data length
 * @param key Key
 * @param iv IV
 * @param plaintext Decrypted data
 *
 * @return Decrypted data length
 */
int _decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
    unsigned char *iv, unsigned char *plaintext) {

  EVP_CIPHER_CTX *ctx;

  int len;

  int plaintext_len;

  /* Create and initialise the context */
  if(!(ctx = EVP_CIPHER_CTX_new())) {
      
      handleErrors();
      return 0;
  }

  /* Initialise the decryption operation. IMPORTANT - ensure you use a key
   * and IV size appropriate for your cipher
   * In this example we are using 256 bit AES (i.e. a 256 bit key). The
   * IV size for *most* modes is the same as the block size. For AES this
   * is 128 bits */
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
      
    handleErrors();
    return 0;
  }

  /* Provide the message to be decrypted, and obtain the plaintext output.
   * EVP_DecryptUpdate can be called multiple times if necessary
   */
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
      
    handleErrors();
    return 0;
  }
  plaintext_len = len;

  /* Finalise the decryption. Further plaintext bytes may be written at
   * this stage.
   */
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
      
      handleErrors();
      return 0;
  }
  plaintext_len += len;

  /* Clean up */
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}

/**
 * Encrypt KSNet package
 *
 * @param kcr Pointer to ksnetCryptClass object
 * @param package Package to send
 * @param package_len Package length
 * @param buffer Buffer to encrypt, enough to hold encrypted data (package_len + 16)
 * @param [out] encrypt_len Length of encrypted data
 *
 * @return If buffer = NULL this function return allocated pointer to encrypted
 *         buffer with data. Should be free after use. Other way it return
 *         pointer to input buffer.
 */
void *ksnEncryptPackage(ksnCryptClass *kcr, void *package,
                        size_t package_len, void *buffer, size_t *encrypt_len) {

    size_t ptr = 0;

    // Calculate encrypted length
    *encrypt_len = ( ((package_len + 1) / kcr->blocksize) +
                     ((package_len + 1) % kcr->blocksize ? 1 : 0) ) * kcr->blocksize;

    // Create buffer if it NULL
    if(buffer == NULL) {
        buffer = malloc(*encrypt_len + sizeof(uint16_t));
    }

    // Fill buffer
    *((uint16_t*) buffer) = package_len; ptr += sizeof(uint16_t);
    memcpy(buffer + ptr, package, package_len);
    int free_buf_len = *encrypt_len - package_len;
    if(free_buf_len) {
        memset(buffer + ptr + package_len, 0, free_buf_len);
    }

    // Encrypt the package
    #ifdef DEBUG_KSNET
    ksnet_printf( & ((ksnetEvMgrClass*)kcr->ke)->ksn_cfg, DEBUG_VV,
                "%sEncrypt:%s %d bytes to %d bytes buffer ...\n",
                ANSI_BROWN, ANSI_NONE, package_len, (int)(*encrypt_len));
    #endif
    *encrypt_len = _encrypt(package, package_len, kcr->key, kcr->iv,
                            buffer + ptr);

    *encrypt_len += ptr;

    return buffer;
}

/**
 * Decrypt received package
 *
 * @param kcr Pointer to ksnetCryptClass object
 * @param package Pointer to received package data
 * @param package_len Package data length
 * @param [out] decrypt_len Decrypted length of package data
 *
 * @return Pointer to decrypted package data
 */
void *ksnDecryptPackage(ksnCryptClass *kcr, void* package,
                        size_t package_len, size_t *decrypt_len) {

    size_t ptr = 0;

    *decrypt_len = *((uint16_t*)package); ptr += sizeof(uint16_t);
    unsigned char *decrypted = malloc(package_len - ptr); //*decrypt_len + 1);

    // Decrypt the package
    #ifdef DEBUG_KSNET
    ksnet_printf( & ((ksnetEvMgrClass*)kcr->ke)->ksn_cfg, DEBUG_VV,
                "%sDecrypt:%s %d bytes from %d bytes package ...\n",
                ANSI_BROWN, ANSI_NONE, *decrypt_len, package_len - ptr);
    #endif
    *decrypt_len = _decrypt(package + ptr, package_len - ptr, kcr->key, kcr->iv,
        decrypted);
    
    // Copy decrypted data (if decrypted, or 
    if(*decrypt_len) { 

        // Add a NULL terminator. We are expecting printable text
        decrypted[*decrypt_len] = '\0';

        // Copy and free decrypted buffer
        memcpy(package + ptr, decrypted, *decrypt_len + 1);
    }
    
    // If data not decrypted - return input package with package len
    else {
        ptr = 0;
        *decrypt_len = package_len;
    }
    
    free(decrypted);

    return package + ptr;
}

/**
 * Simple check if the packet is encrypted
 * 
 * @param data
 * @param package_len
 * @return 
 */
int ksnCheckEncrypted(void *package, size_t package_len) {
    
    size_t ptr = 0;
    size_t decrypt_len = *((uint16_t*)package); ptr += sizeof(uint16_t);  
    
    return decrypt_len  && decrypt_len < package_len && !((package_len - ptr) % BLOCK_SIZE);
}
