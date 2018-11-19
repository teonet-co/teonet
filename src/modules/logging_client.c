/**
 * \file   logging_client.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet logging client module
 *
 * Created on May 30, 2018, 1:33 PM
 */

#include "ev_mgr.h"
#include "logging_client.h"

#define MODULE "logging_client"
#define kev ((ksnetEvMgrClass*)(ke))
//#define MAP_SIZE_DEFAULT 10

// Logging Async call label
static const uint32_t ASYNC_LABEL = 0xAA77AA77; 

/**
 * Logging servers Maps Foreach user data
 */
typedef struct teoLoggingClientSendData {

    const teoLoggingClientClass *lc;
    void *data; 
    size_t data_length;

} teoLoggingClientSendData;

/**
 * Add logger server peer name to logger servers map
 * 
 * @param ls Pointer to teoLoggingClientClass
 * @param peer Logger server peer name
 */
static void teoLoggingClientAddServer(teoLoggingClientClass *lc, 
        const char *peer) {
    ksnetEvMgrClass *ke = lc->ke;
    if(teoMapAdd(lc->map, (void*)peer, strlen(peer)+1, NULL, 0) != (void*)-1) {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                "add logging server peer '%s'\n", peer);
        #endif
    }
    else {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, ERROR_M, 
                "can't add logging server peer '%s'\n", peer);
        #endif        
    }
}

/**
 * Remove logger server peer name from logger servers map
 * 
 * @param ls Pointer to teoLoggingClientClass
 * @param peer Logger server peer name
 */
static void teoLoggingClientRemoveServer(teoLoggingClientClass *lc, 
        const char *peer) {
    ksnetEvMgrClass *ke = lc->ke;
    if(!teoMapDelete(lc->map, (void*)peer, strlen(peer)+1)) {
        #ifdef DEBUG_KSNET
        ksn_printf(ke, MODULE, DEBUG /*DEBUG_VV*/, 
                "remove logging server peer '%s'\n", peer); // \TODO set DEBUG_VV
        #endif
    }
}

/**
 * Send log data to one logging server
 * 
 * @param m Pointer to teoMap
 * @param idx Index
 * @param el Pointer to map element data
 * @param user_data Pointer to user data 
 * @return 
 */
static int teoLoggingClientSendOne(teoMap *m, int idx, teoMapElementData *el, 
        void* user_data) {
    const teoLoggingClientSendData *data = user_data;
    const ksnetEvMgrClass *ke = data->lc->ke;
    const char* peer = el->data;
    ksnCoreSendCmdto(ke->kc, (char*)peer, CMD_LOGGING, data->data, 
            data->data_length);
    return 0;
}

// Send log data to logging servers
static void teoLoggingClientSendAll(ksnetEvMgrClass *ke, void *data, 
        size_t data_length) {
    const teoLoggingClientClass *lc = ke->lc;
    if(lc) {
        teoLoggingClientSendData d = { lc, data, data_length };
        teoMapForeach(lc->map, teoLoggingClientSendOne, &d);
    }
}

// Event loop to gab teonet events
static void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
        size_t data_length, void *user_data) {

    int processed = 0;
    const ksnCorePacketData *rd = (ksnCorePacketData *) data;
    switch(event) {

        // Remove logging server from map
        case EV_K_DISCONNECTED:
            teoLoggingClientRemoveServer(ke->lc, rd->from);
            break;

        // Add logging server to map
        case EV_K_RECEIVED:
            if(rd->cmd == CMD_LOGGING && rd->data_len == 0) {
                teoLoggingClientAddServer(ke->lc, rd->from);
                processed = 1;
            }
            break;
            
        // Async event from teoLoggingClientSend (kns_printf)
        case EV_K_ASYNC:
            if(user_data && *(uint32_t*)user_data == ASYNC_LABEL) {
                teoLoggingClientSendAll(ke, data, data_length);
                processed = 1;
            }
            break;

        default:
            break;
    }

    // Call parent event loop
    if(ke->lc->event_cb != NULL && !processed)
        ((event_cb_t)ke->lc->event_cb)(ke, event, data, data_length, user_data);
}

// Logging client initialize
teoLoggingClientClass *teoLoggingClientInit(void *ke) {
    if(kev->ksn_cfg.log_disable_f) {
        #ifdef DEBUG_KSNET
        ksn_puts(kev, MODULE, DEBUG, "disable send logs to logging servers");
        #endif
        return NULL;
    }

    teoLoggingClientClass *lc = malloc(sizeof(teoLoggingClientClass));
    lc->ke = ke;
    lc->map = teoMapNew(MAP_SIZE_DEFAULT, 1);
    lc->event_cb = kev->event_cb;
    kev->event_cb = event_cb;

    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "have been initialized");
    #endif

    return lc;
}

// Logging client destroy and free allocated memory
void teoLoggingClientDestroy(teoLoggingClientClass *lc) {
    if(lc) {
        ksnetEvMgrClass *ke = lc->ke;
        teoMapDestroy(lc->map);
        ke->event_cb = lc->event_cb;
        free(lc);
        ke->lc = NULL;

        #ifdef DEBUG_KSNET
        ksn_puts(ke, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                "have been de-initialized");
        #endif
    }
}

// Send log data to logging servers
void teoLoggingClientSend(void *ke, const char *message) {
    if(!kev->ksn_cfg.log_disable_f) {
        if(kev->ksn_cfg.send_all_logs_f || message[0] == '#' || strstr(message,": ### "))
            ksnetEvMgrAsync(ke, (void*)message, strlen(message)+1, 
                    (void*)&ASYNC_LABEL);
    }
}
