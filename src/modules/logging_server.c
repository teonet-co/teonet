/**
 * \file   logging_server.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet logging server module
 *
 * Created on May 30, 2018, 1:33 PM
 */

#include <syslog.h>
#include "ev_mgr.h"
#include "logging_server.h"
#include "modules/teodb_com.h"

#define MODULE "logging_server"
#define kev ((ksnetEvMgrClass*)(ke))
typedef void (*event_cb_t)(struct ksnetEvMgrClass *ke, ksnetEvMgrEvents event, 
        void *data, size_t data_len, void *user_data);

// Event loop to gab teonet events
static void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event,
        void *data, size_t data_len, void *user_data) {
    
    int processed = 0;
    const ksnCorePacketData *rd = (ksnCorePacketData *) data;
    switch(event) {

        // Send CMD_LOGGING command to connected peer
        case EV_K_CONNECTED:
            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG_VV,
                    "Peer '%s' connected\n", (char*) rd->from);
            #endif
            ksnCoreSendCmdto(ke->kc, rd->from, CMD_LOGGING, NULL, 0);
            break;

        
        case EV_K_DISCONNECTED:
            #ifdef DEBUG_KSNET
            ksn_printf(kev, MODULE, DEBUG_VV,
                    "Peer '%s' disconnected\n", (char*) rd->from);
            #endif
            break;

        case EV_K_RECEIVED:
            if(rd->cmd == CMD_LOGGING && rd->data_len) {
                
                // Show log message
                printf("%s: %s\n", rd->from, (char*)rd->data);
                
                // Add log to syslog
                syslog(LOG_INFO, "TEO_LOGGING: %s: %s", rd->from, (char*)rd->data);
                
                // Send logging event to process it at user level 
                if(ke->ls->event_cb != NULL)
                    ((event_cb_t)ke->ls->event_cb)(ke, EV_K_LOGGING, data, data_len, NULL);
                
                processed = 1;
            }
            break;

        default:
            break;
    }

    // Call parent event loop
    if(ke->ls->event_cb != NULL && !processed)
        ((event_cb_t)ke->ls->event_cb)(ke, event, data, data_len, user_data);
}

// Logging server initialize
teoLoggingServerClass *teoLoggingServerInit(void *ke) {
    if(!kev->ksn_cfg.logging_f) return NULL;

    teoLoggingServerClass *ls = malloc(sizeof(teoLoggingServerClass));
    ls->ke = ke;
    ls->event_cb = kev->event_cb;
    kev->event_cb = event_cb;

    #ifdef DEBUG_KSNET
    ksn_puts(kev, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
            "have been initialized");
    #endif

    return ls;
}

// Logging server destroy and free allocated memory
void teoLoggingServerDestroy(teoLoggingServerClass *ls) {
    if(ls) {
        ksnetEvMgrClass *ke = ls->ke;
        ke->event_cb = ls->event_cb;
        free(ls);
        ke->ls = NULL;

        #ifdef DEBUG_KSNET
        ksn_puts(ke, MODULE, DEBUG /*DEBUG_VV*/, // \TODO set DEBUG_VV
                "have been de-initialized");
        #endif
    }
}
