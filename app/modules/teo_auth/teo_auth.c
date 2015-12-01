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

#include "teo_auth.h"


/**
 * Initialize Teonet authenticate module
 * 
 * @return 
 */
teoAuthClass *teoAuthInit() {
    
    teoAuthClass *ta = malloc(sizeof(teoAuthClass));
    
    return ta;
}


int teoAuthCommand(const char *method, const char *url, const char *data, 
        const char *headers, command_callback cb) {
    
    if(strcmp(url, "register-client")) {
        
    }
    
    else if(strcmp(url, "register")) {
        
    }
    
    else if(strcmp(url, "login")) {
        
    }
    
    else if(strcmp(url, "refresh")) {
        
    }
}

/**
 * Destroy Teonet authenticate module
 * 
 * @param ta
 */
void teoAuthDestroy(teoAuthClass *ta) {
    
    if(ta != NULL) {
        free(ta);
    }
}

