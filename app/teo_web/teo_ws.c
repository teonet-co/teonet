/** 
 * File:   teo_ws.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet websocket L0 connector module
 *
 * Created on November 8, 2015, 3:58 PM
 */

#include <stdlib.h>
#include <stdio.h>

#include "teo_ws.h"
#include "embedded/jsmn/jsmn.h"
#include "utils/rlutil.h"

// Local functions
void teoWSDestroy(teoWSClass *kws);
int teoWSevHandler(teoWSClass *kws, int ev, void *nc, void *data, size_t data_length);
int teoWSprocessMsg(teoWSClass *kws, void *nc, void *data, size_t data_length);

/**
 * Pointer to mg_connection structure
 */
#define nc ((struct mg_connection *)nc_p)
/**
 * Pointer to ksnet_cfg structure
 */
#define conf &((ksnetEvMgrClass*)kws->kh->ke)->ksn_cfg
/**
 * This module label
 */
#define MODULE_LABEL "%sWebsocket L0:%s "

/**
 * Initialize teonet websocket module
 * 
 * @param kh Pointer to ksnHTTPClass
 * 
 * @return 
 */
teoWSClass* teoWSInit(ksnHTTPClass *kh) {
    
    #ifdef DEBUG_KSNET
    ksnet_printf(&((ksnetEvMgrClass*)kh->ke)->ksn_cfg, DEBUG,
            MODULE_LABEL
            "Initialize\n",
            ANSI_YELLOW, ANSI_NONE);
    #endif
    
    teoWSClass *this = malloc(sizeof(teoWSClass));
    this->kh = kh;
    
    this->destroy = teoWSDestroy;
    this->handler = teoWSevHandler;
    this->processMsg = teoWSprocessMsg;
    
    return this;
}

/**
 * Destroy teonet HTTP module]
 * @param kws Pointer to teoWSClass
 */
void teoWSDestroy(teoWSClass *kws) {
    
    #ifdef DEBUG_KSNET
    ksnet_printf(conf, DEBUG,
            MODULE_LABEL
            "Destroy\n", 
            ANSI_YELLOW, ANSI_NONE);
    #endif
    
    if(kws != NULL)
        free(kws);
}

/**
 * Teonet L0 websocket event handler
 * 
 * @param kws Pointer to teoWSClass
 * @param ev Event
 * @param nc_p Pointer to mg_connection structure
 * @param data Websocket data
 * @param data_length Websocket data length
 * 
 * @return True if event processed
 */
int teoWSevHandler(teoWSClass *kws, int ev, void *nc_p, void *data, 
        size_t data_length) {
    
    int processed = 0;
    
    // Check websocket events
    switch(ev) {
        
        // Websocket message.
        case MG_EV_WEBSOCKET_FRAME: {
            
            processed = kws->processMsg(kws, nc_p, data, data_length);
                    
        } break;
        
        default:
            break;
    }
    
    return processed;
}

/**
 * Check json key equalize to string
 * 
 * @param json Json string
 * @param tok Jsmt json token
 * @param s String to compare
 * 
 * @return Return 0 if found
 */
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    
    if (tok->type == JSMN_STRING && 
        (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        
        return 0;
    }
    
    return -1;
}

/**
 * Process websocket message
 * 
 * @param kws Pointer to teoWSClass
 * @param nc_p Pointer to mg_connection structure
 * @param data Websocket data
 * @param data_length Websocket data length
 * 
 * @return  True if message processed
 */
int teoWSprocessMsg(teoWSClass *kws, void *nc_p, void *data, 
        size_t data_length) {
    
    int processed = 0;

    // Json parser data
    jsmn_parser p;
    jsmntok_t t[128]; // We expect no more than 128 tokens
        
    // Parse json
    jsmn_init(&p);
    int r = jsmn_parse(&p, data, data_length, t, sizeof(t)/sizeof(t[0]));    
    if(r < 0) {
        
        // printf("Failed to parse JSON: %d\n", r);
        return 0;
    }
    
    // Assume the top-level element is an object 
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        
        // printf("Object expected\n");
        return 0;
    }
    
    // Teonet L0 websocket connector json command string format:
    // { "cmd": 1, "to": "peer_name", "data": "Command data" }
    //
    // Loop over all keys of the root object
    enum KEYS {
        CMD = 0x1,
        TO = 0x2,
        CMD_DATA = 0x4,
        ALL_KEYS = CMD | TO | CMD_DATA
    };
    int i, cmd = 0, keys = 0;
    char *to = NULL, *cmd_data = NULL;
    for (i = 1; i < r; i++) {
        
        if(jsoneq(data, &t[i], "cmd") == 0) {
            
            char *cmd_s = strndup((char*)data + t[i+1].start, 
                    t[i+1].end-t[i+1].start);
            cmd = atoi(cmd_s);
            free(cmd_s);
            keys |= CMD;
            i++;
                
        } else if(jsoneq(data, &t[i], "to") == 0) {
            
            to = strndup((char*)data + t[i+1].start, 
                    t[i+1].end-t[i+1].start);
            keys |= TO;
            i++;
            
        } else if(jsoneq(data, &t[i], "data") == 0) {
            
            cmd_data = strndup((char*)data + t[i+1].start, 
                    t[i+1].end-t[i+1].start);
            keys |= CMD_DATA;
            i++;
        }        
    }
    
    // Skip this request if not all keys was send
    if(keys != ALL_KEYS) {
        
        if(to != NULL && keys & TO) free(to);
        if(cmd_data != NULL && keys & CMD_DATA) free(cmd_data);
        
        return 0;
    }
    
    // Check for L0 websocket Login command
    // { "cmd": 0, "to": "", "data": "client_name" }
    if(cmd == 0 && to[0] == 0 && cmd_data[0] != 0) {
        
        #ifdef DEBUG_KSNET
        ksnet_printf(conf, DEBUG,
                MODULE_LABEL 
                "Login from \"%s\" received\n", 
                ANSI_YELLOW, ANSI_NONE, cmd_data);
        #endif
        processed = 1;
    }
    
    // Check for L0 websocket Peers command
    // { "cmd": 72, "to": "peer_name", "data": "" }
    else if(cmd == 72 && to[0] != 0 && cmd_data[0] == 0) {
        
        #ifdef DEBUG_KSNET
        ksnet_printf(conf, DEBUG,
                MODULE_LABEL
                "Peers command to \"%s\" peer received\n", 
                ANSI_YELLOW, ANSI_NONE, to);
        #endif
        processed = 1;
    }
    
    
    // Check for L0 websocket ECHO command
    // { "cmd": 65, "to": "peer_name", "data": "hello" }
    else if(cmd == 65 && to[0] != 0 && cmd_data[0] != 0) {
        
        #ifdef DEBUG_KSNET
        ksnet_printf(conf, DEBUG,
                MODULE_LABEL
                "Echo command to \"%s\" peer with message \"%s\" received\n", 
                ANSI_YELLOW, ANSI_NONE, to, cmd_data);
        #endif
        processed = 1;
    }
    
    free(to);
    free(cmd_data);
    
    return processed;
}
