/*
 * \file:   async_calls.h
 * \author: max
 *
 * Created on June 25, 2018, 8:46 PM
 */

#ifndef ASYNC_CALLS_H
#define ASYNC_CALLS_H

#ifdef __cplusplus
extern "C" {
#endif

void sendCmdToBinaryA(void *ke, void *peer, uint8_t cmd, void *data,
        size_t data_length);

void sendToSscrA(void *ke, uint8_t event, void *data, size_t data_length,
        uint8_t cmd);

void sendCmdAnswerToBinaryA(void *ke, void *rd, uint8_t cmd, void *data,
        size_t data_length);

//void subscribeA
#ifdef __cplusplus
}
#endif

#endif /* ASYNC_CALLS_H */

