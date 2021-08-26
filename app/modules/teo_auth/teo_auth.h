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
#include <pthread.h>

#include <pbl.h>

#include "../teo_web/teo_web.h"

/**
 * Teonet authentication module class structure
 */
typedef struct teoAuthClass {
    ksnHTTPClass *kh; ///< Pointer to ksnHTTPClass
    PblList* list; ///< Commands list
    pthread_mutex_t async_mutex; ///< Command list mutex    
    pthread_t tid; ///< Authentication module thread id
    pthread_mutex_t cv_mutex; ///< Command list condition variables mutex
    pthread_cond_t cv_threshold; ///< Command list condition variable    
    
    int stop; ///< Stop Authentication module thread server flag
    int stopped; ///< Authentication module thread is stopped
    int running; ///< Authentication module thread running state: 1 - running; 0 - waiting
} teoAuthClass;

//typedef void (*command_callback)(void *error, void *success);
typedef void (*command_callback)(void *nc_p, char* err, char *result);

/**
 * Teonet authentication request list data structure
 */
typedef struct teoAuthData {
    char *method;
    char *url;
    char *data; 
    char *headers;
    
    void *nc_p;
    command_callback callback;
} teoAuthData;


#ifdef	__cplusplus
extern "C" {
#endif

teoAuthClass *teoAuthInit(ksnHTTPClass *kh);
void teoAuthDestroy(teoAuthClass *ta);

int teoAuthProcessCommand(teoAuthClass *ta, const char *method, const char *url, 
        const char *data, const char *headers, void *nc_p, 
        command_callback callback);

#ifdef	__cplusplus
}
#endif

#endif	/* TEO_AUTH_H */

