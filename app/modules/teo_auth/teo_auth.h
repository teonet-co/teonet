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

typedef struct teoAuthClass {
    
} teoAuthClass;

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

