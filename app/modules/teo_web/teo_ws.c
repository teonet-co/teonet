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

#include "embedded/jsmn/jsmn.h"
#include "utils/rlutil.h"

#include "../teo_auth/teo_auth.h"
#include "teo_ws.h"

// Local functions
static void teoWSDestroy(teoWSClass *kws);
static teoLNullConnectData *teoWSadd(teoWSClass *kws, void *nc_p, 
                            const char *server, const int port, char *login);
static int teoWSremove(teoWSClass *kws, void *nc_p);
static int teoWSevHandler(teoWSClass *kws, int ev, void *nc, void *data, size_t data_length);
static ssize_t teoWSLNullsend(teoWSClass *kws, void *nc_p, int cmd, 
        const char *to_peer_name, void *data, size_t data_length);
static int teoWSprocessMsg(teoWSClass *kws, void *nc, void *data, size_t data_length);

static void send_auth_answer(void *nc_p, char* err, char *result);

/**
 * Pointer to mg_connection structure
 */
#define nc ((struct mg_connection *)nc_p)
/**
 * Pointer to ksnet_cfg structure
 */
//#define ksn_conf &((ksnetEvMgrClass*)kws->kh->ke)->ksn_cfg
#define kev ((ksnetEvMgrClass*)kws->kh->ke)
/**
 * This module label
 */
#define MODULE _ANSI_YELLOW "websocket_L0" _ANSI_NONE

/**
 * Initialize teonet websocket module
 * 
 * @param kh Pointer to ksnHTTPClass
 * 
 * @return 
 */
teoWSClass* teoWSInit(ksnHTTPClass *kh) {
    
    #ifdef DEBUG_KSNET
//    ksnet_printf(&((ksnetEvMgrClass*)kh->ke)->ksn_cfg, DEBUG,
//            MODULE_LABEL
//            "Initialize\n",
//            ANSI_YELLOW, ANSI_NONE);
    ksn_puts(((ksnetEvMgrClass*)kh->ke), MODULE, DEBUG, "initialize");
    #endif
    
    teoWSClass *this = malloc(sizeof(teoWSClass));
    this->kh = kh;
    this->map = pblMapNewHashMap();
    
    this->destroy = teoWSDestroy;
    this->add = teoWSadd;
    this->remove = teoWSremove;
    this->handler = teoWSevHandler;
    this->send = teoWSLNullsend;
    this->processMsg = teoWSprocessMsg;
    
    return this;
}

#define teoLNullDisconnectThis(con) { free(con->user_data); teoLNullDisconnect(con); }

/**
 * Destroy teonet HTTP module]
 * @param kws Pointer to teoWSClass
 */
static void teoWSDestroy(teoWSClass *kws) {
        
    if(kws != NULL) {

        #ifdef DEBUG_KSNET
//        ksnet_printf(ksn_conf, DEBUG,
//                MODULE_LABEL
//                "Destroy\n", 
//                ANSI_YELLOW, ANSI_NONE);
        ksn_puts(kev, MODULE, DEBUG, "Destroy");
        #endif

        // Disconnect all connected clients and stop it watchers
        PblIterator *it = pblMapIteratorReverseNew(kws->map);
        if (it != NULL) {
            while (pblIteratorHasPrevious(it)) {
                void *entry = pblIteratorPrevious(it);
                teoWSmapData *td = pblMapEntryValue(entry);
                ev_io_stop(((ksnetEvMgrClass*)kws->kh->ke)->ev_loop, &td->w); // stop watcher
                teoLNullDisconnectThis(td->con); // disconnect connection to L0 server
            }
            pblIteratorFree(it);
        }
        
        pblMapFree(kws->map);
        free(kws);
    }
}

/**
 * L0 server teonet L0 connect user data
 */
typedef struct teoLNullConnectUserData {
    
    void *nc_p; ///< Pointer to mg_connection structure
    teoWSClass *kws; ///< Pointer to teoWSClass
    
} teoLNullConnectUserData;

/**
 * Read data from L0 server
 * 
 * @param loop
 * @param w
 * @param revents
 */
static void read_cb(struct ev_loop *loop, struct ev_io *w, int revents) {
    
    teoLNullConnectData *con = w->data;
    #define kev_t \
    ((ksnetEvMgrClass*)((teoLNullConnectUserData*)con->user_data)->kws->kh->ke)
    void *nc_p = ((teoLNullConnectUserData*)con->user_data)->nc_p;
    
    for(;;) {
        
        // Read data
        ssize_t rc = teoLNullRecv(con);
        
        // Process received data
        if(rc > 0) {
            
            teoLNullCPacket *cp = (teoLNullCPacket*) con->read_buffer;
            char *data = cp->peer_name + cp->peer_name_length;
            
            #ifdef DEBUG_KSNET
//            ksnet_printf(&ksn_conf_t, DEBUG_VV,
//                MODULE_LABEL
//                "Receive %d bytes: %d bytes data from L0 server, "
//                "from peer %s, cmd = %d\n",
//                ANSI_YELLOW, ANSI_NONE, 
//                (int)rc, cp->data_length, cp->peer_name, cp->cmd);
            ksn_printf(kev_t, MODULE, DEBUG_VV,
                "receive %d bytes: %d bytes data from L0 server, "
                "from peer %s, cmd = %d\n",
                (int)rc, cp->data_length, cp->peer_name, cp->cmd);
            #endif
                        
            // Define json type of data field
            //
            char *beg, *end;
            jsmntype_t type = JSMN_UNDEFINED;
            size_t num_of_tags = get_num_of_tags(data, cp->data_length);
            if(num_of_tags) {
                
                // Parse json
                jsmntok_t *t = malloc(num_of_tags * sizeof(jsmntok_t));
                //
                jsmn_parser p;
                jsmn_init(&p);
                int r = jsmn_parse(&p, data, cp->data_length, t, num_of_tags);    
                if(!(r < 1)) type = t[0].type;
                //
                free(t);
            }
            
            if(type == JSMN_STRING || type == JSMN_UNDEFINED) beg = end = "\"";
            else beg = end = "";             
            
            char *data_str = data;
            int data_len = cp->data_length;
            if(cp->cmd == CMD_L_PEERS_ANSWER) {
                
                // Convert binary peer list data to json
                ksnet_arp_data_ar *arp_data_ar = (ksnet_arp_data_ar *) data;
                data_str = malloc(sizeof(arp_data_ar->arp_data[0]) * 2 * arp_data_ar->length);
                int ptr = sprintf(data_str, "{ \"length\": %d, \"arp_data_ar\": [ ", arp_data_ar->length);
                int i = 0;
                for(i = 0; i < arp_data_ar->length; i++) {
                    ptr += sprintf(data_str + ptr, 
                            "%s{ "
                            "\"name\": \"%s\", "
                            "\"mode\": %d, "
                            "\"addr\": \"%s\", "
                            "\"port\": %d, "
                            "\"triptime\": %.3f,"
                            "\"uptime\": %.3f"
                            " }", 
                            i ? ", " : "", 
                            arp_data_ar->arp_data[i].name,
                            arp_data_ar->arp_data[i].data.mode,
                            arp_data_ar->arp_data[i].data.addr,
                            arp_data_ar->arp_data[i].data.port,
                            arp_data_ar->arp_data[i].data.last_triptime,
                            arp_data_ar->arp_data[i].data.connected_time // uptime
                    );
                }
                sprintf(data_str + ptr, " ] }");
                data_len = strlen(data_str);
                beg = end = ""; 
            }
            else if(cp->cmd == CMD_L_L0_CLIENTS_ANSWER) {
                
                // Convert binary client list data to json
                teonet_client_data_ar *client_data_ar = (teonet_client_data_ar *) data;
                data_str = malloc(sizeof(client_data_ar->client_data[0]) * 2 * client_data_ar->length);
                int ptr = sprintf(data_str, "{ \"length\": %d, \"client_data_ar\": [ ", client_data_ar->length);
                int i = 0;
                for(i = 0; i < client_data_ar->length; i++) {
                    ptr += sprintf(data_str + ptr, 
                            "%s{ "
                            "\"name\": \"%s\""
                            " }", 
                            i ? ", " : "", 
                            client_data_ar->client_data[i].name
                    );
                }
                sprintf(data_str + ptr, " ] }");
                data_len = strlen(data_str);
                beg = end = ""; 
            }
            else if(cp->cmd == CMD_L_SUBSCRIBE_ANSWER) {
                
                teoSScrData *sscr_data = (teoSScrData *) data;
                data_len =  cp->data_length - sizeof(teoSScrData);
                int b64_data_size = 2 * data_len + 4;
                char *b64_data = malloc(b64_data_size);
                mg_base64_encode((const unsigned char*)sscr_data->data, data_len, b64_data);
                data_str = ksnet_formatMessage(
                    "{ \"ev\": %d, \"cmd\": %d, \"data\": \"%s\" }", 
                    sscr_data->ev, sscr_data->cmd, b64_data);
                data_len = strlen(data_str);
                free(b64_data);
                beg = end = ""; 
            }
            else if(type == JSMN_UNDEFINED) {
                
                data_len = cp->data_length;
                int b64_data_size = 2 * data_len + 4;
                char *b64_data = malloc(b64_data_size);
                mg_base64_encode((const unsigned char*)data, data_len, b64_data);
                data_str = b64_data; //strdup("undefined"); 
                data_len = strlen(data_str);
            }

            // Create json data and send it to websocket client
            size_t data_json_len = 128 + cp->data_length;
            char *data_json = malloc(data_json_len);
            data_json_len = snprintf(data_json, data_json_len, 
                "{ \"cmd\": %d, \"from\": \"%s\", \"data\": %s%.*s%s }",
                cp->cmd, cp->peer_name, 
                beg, data_len, data_str, end
            );
            
            #ifdef DEBUG_KSNET
//            ksnet_printf(&ksn_conf_t, DEBUG_VV,
//                MODULE_LABEL
//                "Send %d bytes JSON: %s to L0 client\n",
//                ANSI_YELLOW, ANSI_NONE, 
//                data_json_len, data_json);
            ksn_printf(kev_t, MODULE, DEBUG_VV,
                "send %d bytes JSON: %s to L0 client\n",
                data_json_len, data_json);
            #endif

            mg_send_websocket_frame(nc_p, WEBSOCKET_OP_TEXT, data_json, 
                    data_json_len);                        
            
            if(data_str != data) free(data_str);
            free(data_json);
        }
        else break;
    }
    #undef kev_t
}

/**
 * Connect WS client with L0 server, add it to connected map and create READ 
 * watcher
 * 
 * @param kws Pointer to teoWSClass
 * @param nc_p Pointer to websocket connector
 * @param server L0 Server name or IP
 * @param port L0 Server port
 * @param login L0 server login
 * 
 * @return Pointer to teoLNullConnectData or NULL if error
 */
static teoLNullConnectData *teoWSadd(teoWSClass *kws, void *nc_p, 
        const char *server, const int port, char *login) {
    
    int rv = -1;
    
    // Connect to L0 server
    teoLNullConnectData *con = teoLNullConnect(server, port);
    con->user_data = malloc(sizeof(teoLNullConnectUserData));
    ((teoLNullConnectUserData*)con->user_data)->kws = kws;
            
    if(con != NULL && con->fd > 0) {
        
        ssize_t snd = teoLNullLogin(con, login);
        if(snd != -1) {

            // Add to map
            int rc;
            teoWSmapData tws_map_data, *td;
            tws_map_data.con = con;
            size_t valueLength;
            if((rc = pblMapAdd(kws->map, (void*)&nc_p, sizeof(nc_p), 
                    &tws_map_data, sizeof(tws_map_data))) >= 0) {   
               
                if((td = pblMapGet(kws->map, (void*)&nc_p, sizeof(nc_p), 
                        &valueLength)) != NULL) {
                    
                    // Create and start fd watcher
                    ev_init (&td->w, read_cb);
                    ev_io_set (&td->w, td->con->fd, EV_READ);
                    td->w.data = con;
                    ((teoLNullConnectUserData*)con->user_data)->nc_p = nc_p;
                    ev_io_start (((ksnetEvMgrClass*)kws->kh->ke)->ev_loop, 
                            &td->w);                

                    #ifdef DEBUG_KSNET
//                    ksnet_printf(ksn_conf, DEBUG,
//                            MODULE_LABEL
//                            "WS client %p has connected to L0 server ...\n", 
//                            ANSI_YELLOW, ANSI_NONE, nc_p);
                    ksn_printf(kev, MODULE, DEBUG,
                            "WS client %p has connected to L0 server ...\n", 
                            nc_p);
                    #endif

                    // Send websocket welcome message
                    char *WELCOME_NET = ksnet_formatMessage("Hi %s! Welcome to Teonet!", login);
                    mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, WELCOME_NET, strlen(WELCOME_NET) + 1);
                    free(WELCOME_NET);

                    rv = 0;
                }
            }
        }

        // Disconnect from L0 server at error
        if(rv) {
            teoLNullDisconnectThis(con);
            con = NULL;
        }
    } 
    else con = NULL;
    
    return con;
}

/*
 * Disconnect WS client from L0 server, remove it from connected map and stop 
 * READ watcher
 * 
 * @param kws Pointer to teoWSClass
 * @param nc_p Pointer to mg_connection structure
 * 
 * @return Return true at success
 */
static int teoWSremove(teoWSClass *kws, void *nc_p) {
    
    int rv = 0;
    
    size_t valueLength;
    //teoLNullConnectData *con;
    teoWSmapData *td;
    
    if((td = pblMapGet(kws->map, (void*)&nc_p, sizeof(nc_p), &valueLength)) 
            != NULL) { 
        
        ev_io_stop(((ksnetEvMgrClass*)kws->kh->ke)->ev_loop, &td->w); // stop watcher
        teoLNullDisconnectThis(td->con); // disconnect connection to L0 server
        pblMapRemoveFree(kws->map, (void*)&nc_p, sizeof(nc_p), &valueLength);
        
        #ifdef DEBUG_KSNET
//        ksnet_printf(ksn_conf, DEBUG,
//                MODULE_LABEL
//                "WS client %p has disconnected from L0 server ...\n", 
//                ANSI_YELLOW, ANSI_NONE, nc_p);
        ksn_printf(kev, MODULE, DEBUG,
                "WS client %p has disconnected from L0 server ...\n", nc_p);        
        #endif
        
        rv = 1;
    }
    
    return rv;
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
static int teoWSevHandler(teoWSClass *kws, int ev, void *nc_p, void *data, 
        size_t data_length) {
    
    int processed = 0;
    
    // Check websocket events
    switch(ev) {
        
        // Websocket message.
        case MG_EV_WEBSOCKET_FRAME: 
            processed = kws->processMsg(kws, nc_p, data, data_length);
            break;
            
        // Websocket closed    
        case MG_EV_CLOSE:
            processed = kws->remove(kws, nc_p);
            break;
        
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
 * Send command to L0 server
 * 
 * Create L0 clients packet and send it to L0 server
 * 
 * @param con Pointer to teoLNullConnectData
 * @param cmd Command
 * @param peer_name Peer name to send to
 * @param data Pointer to data
 * @param data_length Length of data
 * 
 * @return Length of send data or -1 at error
 */
static ssize_t teoWSLNullsend(teoWSClass *kws, void *nc_p, int cmd, 
        const char *to_peer_name, void *data, size_t data_length) {

    teoWSmapData *td;    
    ssize_t snd = -1;
    size_t valueLength;
    
    if((td = pblMapGet(kws->map, (void*)&nc_p, sizeof(nc_p), &valueLength)) 
            != NULL) { 

        snd = teoLNullSend(td->con, cmd, to_peer_name, data, data_length);
    }
    
    return snd;
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
static int teoWSprocessMsg(teoWSClass *kws, void *nc_p, void *data, 
        size_t data_length) {
    
    int processed = 0;
    
    #ifdef DEBUG_KSNET
//    ksnet_printf(ksn_conf, DEBUG_VV,
//        MODULE_LABEL
//        "Receive %d bytes: from WS client %p\n",
//        ANSI_YELLOW, ANSI_NONE, 
//        data_length, nc_p );
    ksn_printf(kev, MODULE, DEBUG_VV,
        "receive %d bytes: from WS client %p\n",
        data_length, nc_p );
    #endif
    
    
    // Json parser data
    size_t num_of_tags = get_num_of_tags(data, data_length);
    if(!num_of_tags) return 0;
    
    jsmntok_t *t = malloc(num_of_tags * sizeof(jsmntok_t));
        
    // Parse json
    jsmn_parser p;
    jsmn_init(&p);
    int r = jsmn_parse(&p, data, data_length, t, num_of_tags/*sizeof(t)/sizeof(t[0])*/);    
    if(r < 0) {
        
        // This is not JSON string - skip processing
        // printf("Failed to parse JSON: %d\n", r);
        free(t);
        return 0;
    }
    
    // Assume the top-level element is an object 
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        
        // This is not JSON object - skip processing
        // printf("Object expected\n");
        free(t);
        return 0;
    }
    
    // Teonet L0 websocket connector json command string format:
    //
    // { "cmd": 1, "to": "peer_name", "data": "Command data string" }
    // or
    // { "cmd": 1, "to": "peer_name", "data": { Command data object } }
    //
    enum KEYS {
        CMD = 0x1,
        TO = 0x2,
        CMD_DATA = 0x4,
        ALL_KEYS = CMD | TO | CMD_DATA
    };
    int i, cmd = 0, keys = 0;
    char *to = NULL, *cmd_data = NULL;
    // Loop over json keys of the root object and find needle: cmd, to, cmd_data
    for (i = 1; i < r && keys != ALL_KEYS; i++) {
        
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
    free(t);
    
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
//        ksnet_printf(ksn_conf, DEBUG,
//                MODULE_LABEL 
//                "Login from \"%s\" received\n", 
//                ANSI_YELLOW, ANSI_NONE, cmd_data);
        ksn_printf(kev, MODULE, DEBUG,
                "login from \"%s\" received\n", cmd_data);
        #endif

        // Connect to L0 server
        // \todo Send L0 server name and port in Login command or use WS host L0 server
        if(kws->add(kws, nc_p, kws->kh->conf->l0_server_name, 
           kws->kh->conf->l0_server_port, cmd_data) != NULL)        
                processed = 1;
    }
    
    // Check for L0 websocket Peers command
    // { "cmd": 72, "to": "peer_name", "data": "" }
    else if(cmd == CMD_L_PEERS && to[0] != 0 && cmd_data[0] == 0) {
        
        #ifdef DEBUG_KSNET
//        ksnet_printf(ksn_conf, DEBUG_VV,
//                MODULE_LABEL
//                "Peers command to \"%s\" peer received\n", 
//                ANSI_YELLOW, ANSI_NONE, to);
        ksn_printf(kev, MODULE, DEBUG_VV,
                "peers command to \"%s\" peer received\n", to);
        #endif

        // Send request peers command
        if(kws->send(kws, nc_p, cmd, to, NULL, 0) != -1)
            processed = 1;
    }
    
    
    // Check for L0 websocket ECHO command
    // { "cmd": 65, "to": "peer_name", "data": "hello" }
    else if(cmd == CMD_L_ECHO && to[0] != 0 && cmd_data[0] != 0) {
        
        #ifdef DEBUG_KSNET
//        ksnet_printf(ksn_conf, DEBUG_VV,
//                MODULE_LABEL
//                "Echo command to \"%s\" peer with message \"%s\" received\n", 
//                ANSI_YELLOW, ANSI_NONE, to, cmd_data);
        ksn_printf(kev, MODULE, DEBUG_VV,
                "echo command to \"%s\" peer with message \"%s\" received\n", 
                to, cmd_data);
        #endif

        // Send echo command to L0 server
        if(kws->send(kws, nc_p, cmd, to, cmd_data, strlen(cmd_data) + 1) != -1)
            processed = 1;
    }
    
    // Authentication command CMD_L_AUTH
    // { "cmd": 77, "to": "peer_name", "data": { "method": "POST", "url": "register-client", "data": "data", "headers": "headers" } }
    else if(cmd == CMD_L_AUTH && cmd_data[0] != 0) {
        
        #ifdef DEBUG_KSNET
//        ksnet_printf(ksn_conf, DEBUG_VV,
//            MODULE_LABEL
//            "Authentication command to \"%s\" peer, with data \"%s\" was received\n", 
//            ANSI_YELLOW, ANSI_NONE, to, cmd_data);
        ksn_printf(kev, MODULE, DEBUG_VV,
            "authentication command to \"%s\" peer, with data \"%s\" was received\n", 
            to, cmd_data);
        #endif

        // \todo Process Authentication command in Authentication module
        jsmn_parser p;
        size_t cmd_data_len = strlen(cmd_data);
        size_t num_of_tags = get_num_of_tags(cmd_data, data_length);
        if(num_of_tags) {
            
            jsmntok_t *t = malloc(num_of_tags * sizeof(jsmntok_t));        
            jsmn_init(&p);
            int r = jsmn_parse(&p, cmd_data, cmd_data_len, t, num_of_tags
                    /*sizeof(t)/sizeof(t[0])*/);    

            if(!(r < 1)) { //type = t[0].type;
                enum KEYS {
                    METHOD = 0x01,
                    URL = 0x02,
                    DATA = 0x04,
                    HEADERS = 0x08,
                    ALL_KEYS = METHOD | URL | DATA | HEADERS
                };
                int i, keys = 0;
                char *method = NULL, *url = NULL, *data = NULL, *headers = NULL;
                for (i = 1; i < r && keys != ALL_KEYS; i++) {

                    if(jsoneq(cmd_data, &t[i], "method") == 0) {

                        method = strndup((char*)cmd_data + t[i+1].start, 
                                t[i+1].end-t[i+1].start);
                        keys |= METHOD;
                        i++;

                    } else if(jsoneq(cmd_data, &t[i], "url") == 0) {

                        url = strndup((char*)cmd_data + t[i+1].start, 
                                t[i+1].end-t[i+1].start);
                        keys |= URL;
                        i++;

                    } else if(!(keys & DATA) && jsoneq(cmd_data, &t[i], "data") == 0) {

                        data = strndup((char*)cmd_data + t[i+1].start, 
                                t[i+1].end-t[i+1].start);
                        keys |= DATA;
                        i++;

                    } else if(jsoneq(cmd_data, &t[i], "headers") == 0) {

                        headers = strndup((char*)cmd_data + t[i+1].start, 
                                t[i+1].end-t[i+1].start);
                        keys |= HEADERS;
                        i++;
                    }
                }

                // Process and execute authenticate command
                if(ALL_KEYS) {

                    teoAuthProcessCommand(kws->kh->ta, method, url, data, headers,
                            nc_p, send_auth_answer);
                }
            }
            free(t);
        }
        
        processed = 1;
    }
    
    // Send other commands to L0 server
    else if(to[0] != 0) {
        
        #ifdef DEBUG_KSNET
//        ksnet_printf(ksn_conf, DEBUG_VV,
//            MODULE_LABEL
//            "Resend command %d to \"%s\" peer with message \"%s\" received\n", 
//            ANSI_YELLOW, ANSI_NONE, cmd, to, cmd_data);
        ksn_printf(kev, MODULE, DEBUG_VV,
            "resend command %d to \"%s\" peer with message \"%s\" received\n", 
            cmd, to, cmd_data);        
        #endif

        // Send other command
        if(kws->send(kws, nc_p, cmd, to, cmd_data, strlen(cmd_data) + 1) != -1)
            processed = 1;
    }
    
    free(to);
    free(cmd_data);
    
    return processed;
}

/**
 * Send authenticate command answer
 * 
 * @param nc_p
 * @param err
 * @param result
 */
static void send_auth_answer(void *nc_p, char* err, char *result) {
        
    // Send tests answer command
    const char *data_json = "{ \"cmd\": 78, \"from\": \"\", \"data\": %s }";
    size_t data_json_len = strlen(data_json)  + strlen(result);
    char data[data_json_len + 1];
    data_json_len = snprintf(data, data_json_len, data_json, result);
    mg_send_websocket_frame(nc_p, WEBSOCKET_OP_TEXT, data, data_json_len);    
}
