/**
 * \file   logging_client.h
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet logging client module
 *
 * Created on May 30, 2018, 1:33 PM
 */

#ifndef LOGGING_CLIENT_H
#define LOGGING_CLIENT_H

/**
 * Teonet Logging client class data definition
 */
typedef struct teoLoggingClientClass {

    void *ke; // Pointer to ksnEvMgrClass
    teoMap *map; // Logging servers set
    void *event_cb; // Pointer to event callback

} teoLoggingClientClass;


#ifdef __cplusplus
extern "C" {
#endif    

/**
 * Logging client initialize
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @return Pointer to teoLoggingClientClass
 */
teoLoggingClientClass *teoLoggingClientInit(void *ke);

/**
 * Logging client destroy and free allocated memory
 *
 * @param ls Pointer to teoLoggingClientClass
 */
void teoLoggingClientDestroy(teoLoggingClientClass *lc);

/**
 * Add logger server peer name to logger servers map
 * 
 * @param ls Pointer to teoLoggingClientClass
 * @param peer Logger server peer name
 */
void teoLoggingClientAddServer(teoLoggingClientClass *lc, const char *peer);

/**
 * Remove logger server peer name from logger servers map
 * 
 * @param ls Pointer to teoLoggingClientClass
 * @param peer Logger server peer name
 */
void teoLoggingClientRemoveServer(teoLoggingClientClass *lc, const char *peer);

#ifdef __cplusplus
}
#endif

#endif /* LOGGING_CLIENT_H */
