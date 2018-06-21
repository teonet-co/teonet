/**
 * \file   logging_server.h
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet logging server module
 *
 * Created on May 30, 2018, 1:33 PM
 */

#ifndef LOGGING_SERVER_H
#define LOGGING_SERVER_H

/**
 * Teonet Logging server class data definition
 */
typedef struct teoLoggingServerClass {

    void *ke; // Pointer to ksnEvMgrClass
    void *event_cb; // Pointer to event callback
    char *filter;
    
} teoLoggingServerClass;


#ifdef __cplusplus
extern "C" {
#endif    

/**
 * Logging server initialize
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @return Pointer to teoLoggingServerClass
 */
teoLoggingServerClass *teoLoggingServerInit(void *ke);

void teoLoggingServerSetFilter(void *ke, void *filter);
/**
 * Logging server destroy and free allocated memory
 *
 * @param ls Pointer to teoLoggingServerClass
 */
void teoLoggingServerDestroy(teoLoggingServerClass *ls);

#ifdef __cplusplus
}
#endif

#endif /* LOGGING_SERVER_H */
