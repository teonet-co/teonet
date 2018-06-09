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
 * Send log data to logging servers
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @param data Pointer to data
 * @param data_length Data length
 */
void teoLoggingClientSend(void *ke, void *data, size_t data_length);

#ifdef __cplusplus
}
#endif

#endif /* LOGGING_CLIENT_H */
