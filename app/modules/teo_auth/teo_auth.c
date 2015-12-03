/** 
 * File:   teo_auth.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Module to connect to Authentication server AuthPrototype
 *
 * Created on December 1, 2015, 1:51 PM
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "teo_auth.h"


/**
 * Initialize Teonet authenticate module
 * 
 * @return 
 */
teoAuthClass *teoAuthInit() {
    
    teoAuthClass *ta = malloc(sizeof(teoAuthClass));
    ta->list = pblListNewArrayList();
    
    return ta;
}

/**
 * Store authentication command in list
 * 
 * @param ta Pointer to teoAuthClass
 * @param method
 * @param url
 * @param data
 * @param headers
 * 
 * @return 
 */
int teoAuthProcessCommand(teoAuthClass *ta, const char *method, const char *url, 
        const char *data, const char *headers) {
    
    teoAuthData *element = malloc(sizeof(teoAuthData));
    
    element->method = strdup(method);
    element->url = strdup(url);
    element->data = strdup(data);
    element->headers = strdup(headers);
    
    pthread_mutex_lock (&ta->async_mutex);
    pblListAdd(ta->list, element);
    pthread_mutex_unlock (&ta->async_mutex);
    
    // \todo Use pthread_cond_signal to send signal to the waiting thread
    // and the pthread_cond_timedwait to wait in thread
    
    return 1;
}

/**
 * Process authentication command
 * 
 * @param ta Pointer to teoAuthClass
 * @param method
 * @param url
 * @param data
 * @param headers
 * @param cb
 * @return 
 */
int teoAuthProccess(teoAuthClass *ta, const char *method, const char *url, const char *data, 
        const char *headers, command_callback cb) {
    
    if(strcmp(url, "register-client")) {
        
    }
    
    else if(strcmp(url, "register")) {
        
    }
    
    else if(strcmp(url, "login")) {
        
    }
    
    else if(strcmp(url, "refresh")) {
        
    };
    
    return 0;
}

/**
 * Destroy Teonet authenticate module
 * 
 * @param ta
 */
void teoAuthDestroy(teoAuthClass *ta) {
    
    // Loop list and destroy all loaded modules
    PblIterator *it =  pblListIterator(ta->list);
    if(it != NULL) {       
        while(pblIteratorHasNext(it)) {        
            
////            printf("ksnModuleLDestroy before destroy\n");
//            ksnModuleElement *module = pblIteratorNext(it);
//            module->destroy(module->mc);           
        }
        pblIteratorFree(it);
    }
    
    // Free list
    pblListFree(ta->list);
    
    if(ta != NULL) {
        free(ta);
    }
}
