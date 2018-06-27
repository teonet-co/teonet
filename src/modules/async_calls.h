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
    uint8_t test;

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


void sendCmdToBinaryA(void *ke, const char *peer, uint8_t cmd, void *data,
        size_t data_length);

void sendToSscrA(void *ke, uint16_t event, void *data, size_t data_length,
        uint8_t cmd);

void sendCmdAnswerToBinaryA(void *ke, void *rd, uint8_t cmd, void *data,
        size_t data_length);

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

