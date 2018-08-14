/*
 * \file:   async_calls.h
 * \author: max
 *
 * Created on June 25, 2018, 8:46 PM
 */

#ifndef ASYNC_CALLS_H
#define ASYNC_CALLS_H

#include "subscribe.h"

/**
 * Teonet Async class data definition
 */
typedef struct teoAsyncClass {

    void *ke; // Pointer to ksnEvMgrClass
    void *event_cb; // Pointer to event callback
    pthread_t t_id; // Self thread id
    uint8_t test;
    pthread_mutex_t cv_mutex; // Condition variables mutex
    pthread_cond_t cv_threshold; // Condition variable threshold
    pthread_mutex_t async_func_mutex; // Async functions mutex

} teoAsyncClass;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Async module initialize
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @return Pointer to teoAsyncClass
 */
teoAsyncClass *teoAsyncInit(void *ke);

/**
 * Async module destroy and free allocated memory
 *
 * @param ls Pointer to teoAsyncClass
 */
void teoAsyncDestroy(teoAsyncClass *ta);


// # case 1:
void ksnCoreSendCmdtoA(void *ke, const char *peer, uint8_t cmd, void *data,
        size_t data_length);

// # case 2:
void teoSScrSendA(void *ke, uint16_t event, void *data, size_t data_length,
        uint8_t cmd);

// # case 3:
void sendCmdAnswerToBinaryA(void *ke, void *rd, uint8_t cmd, void *data,
        size_t data_length);

// # case 4:
/**
 * Send command to subscribe this host to event at remote peer
 *
 * @param sscr Pointer to teoSScrClass
 * @param peer_name Remote host name
 * @param ev Event
 */
void teoSScrSubscribeA(teoSScrClass *sscr, char *peer_name, uint16_t ev);

#ifdef __cplusplus
}
#endif

#endif /* ASYNC_CALLS_H */

