/** 
 * File:   teo_auth.h
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Module to connect to Authentication server AuthPrototype
 *
 * Created on December 1, 2015, 1:52 PM
 */

#ifndef TEO_AUTH_H
#define	TEO_AUTH_H

#include <stdio.h>
#include <pbl.h>

/**
 * Teonet authentication module class structure
 */
typedef struct teoAuthClass {
    
    PblList* list; ///< Commands list
    pthread_mutex_t async_mutex; ///< Command list mutex    
    
} teoAuthClass;

/**
 * Teonet authentication request list data structure
 */
typedef struct teoAuthData {
    
    const char *method;
    const char *url;
    const char *data; 
    const char *headers;
    
} teoAuthData;

typedef void (*command_callback)(void *error, void *success);

#ifdef	__cplusplus
extern "C" {
#endif

teoAuthClass *teoAuthInit();
void teoAuthDestroy(teoAuthClass *ta);

#ifdef	__cplusplus
}
#endif

#endif	/* TEO_AUTH_H */

