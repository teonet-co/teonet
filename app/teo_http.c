/** 
 * File:   teo_http.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet HTTP/WS Server module
 *
 * Created on October 2, 2015, 12:04 AM
 */

#include "teo_http.h"

enum WS_CMD {
    
    WS_CONNECTED,
    WS_MESSAGE,
    WS_DISCONNECTED
            
};

struct teoweb_data {
    
    uint16_t cmd;
    size_t data_len;
    char data[];
};

/**
 * Mongoose websocket send broadcast
 * 
 * @param nc
 * @param msg
 * @param len
 */
static void ws_broadcast(struct mg_connection *nc, const char *msg, size_t len) {
    
    struct mg_connection *c;
    char buf[500];

    snprintf(buf, sizeof(buf), "%p: %.*s", nc, (int) len, msg);
    for(c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c)) {
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
    }
}

/**
 * Is websocket connected
 * 
 * @param nc
 * @return 
 */
static int is_websocket(const struct mg_connection *nc) {
    
    return nc->flags & MG_F_IS_WEBSOCKET;
}

/**
 * Send async event to teonet library
 * 
 * @param nc
 * @param cmd
 * @param data
 * @param data_len
 */
void teoSendAsync(struct mg_connection *nc, uint16_t cmd, void *data, 
        size_t data_len) {
    
    size_t td_size = sizeof(struct teoweb_data) + data_len;
    struct teoweb_data *td = malloc(td_size);
    td->cmd = cmd; 
    td->data_len = data_len;
    if(data_len) memcpy(td->data, data, data_len);
    ksnetEvMgrAsync(((ksnHTTPClass *)nc->mgr->user_data)->ke, td, td_size, nc); // Send event to teonet
    free(td);
}

/**
 * Mongoose event handler
 * 
 * @param nc
 * @param ev
 * @param ev_data
 */
static void mg_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    
    struct http_message *hm = (struct http_message *) ev_data;
    struct websocket_message *wm = (struct websocket_message *) ev_data;
    ksnHTTPClass *kh = nc->mgr->user_data;
    
    // Check server events
    switch(ev) {

        // Serve HTTP request
        case MG_EV_HTTP_REQUEST:
            mg_serve_http(nc, hm, kh->s_http_server_opts);
            //nc->flags |= MG_F_SEND_AND_CLOSE;
            break;
            
        // New websocket connection. Tell everybody. 
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
            ws_broadcast(nc, "joined", 6); 
            teoSendAsync(nc, WS_CONNECTED, NULL, 0);
        } break;
            
        // New websocket message. Tell everybody.
        case MG_EV_WEBSOCKET_FRAME: {
            ws_broadcast(nc, (char *) wm->data, wm->size);
            teoSendAsync(nc, WS_MESSAGE, wm->data, wm->size);
        } break;
            
        // Disconnect 
        case MG_EV_CLOSE:
            // Disconnect websocket connection. Tell everybody.
            if(is_websocket(nc)) {
                ws_broadcast(nc, "left", 4);
                teoSendAsync(nc, WS_DISCONNECTED, NULL, 0);
            }
            break;
            
        default:
            break;
    }
}

/**
 * Mongoose main thread function
 * 
 * @param kh Pointer to ksnetEvMgrClass
 * 
 * @return 
 */
void* http_thread(void *kh) {

    struct mg_mgr mgr;
    struct mg_connection *nc;

    mg_mgr_init(&mgr, kh);
    nc = mg_bind(&mgr, ((ksnHTTPClass *)kh)->s_http_port, mg_ev_handler);

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);

    printf("Starting web server on port %s ...\n", 
        ((ksnHTTPClass *)kh)->s_http_port);
    
    for (;;) {
      mg_mgr_poll(&mgr, 1000);
      if(((ksnHTTPClass *)kh)->stop) break;
    }
    mg_mgr_free(&mgr);
    
    printf("Web server on port %s stopped.\n", 
        ((ksnHTTPClass *)kh)->s_http_port);
    
    ((ksnHTTPClass *)kh)->stopped = 1;
    
    return NULL;
}

/**
 * Initialize teonet HTTP module
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * 
 * @return 
 */
ksnHTTPClass* ksnHTTPInit(ksnetEvMgrClass *ke) {
    
    ksnHTTPClass *kh = malloc(sizeof(ksnHTTPClass));
    kh->ke = ke;
    
    kh->stop = 0;
    kh->stopped = 0;
    kh->s_http_port = "8000";
    kh->s_http_server_opts.document_root = ".";  // Serve current directory
    kh->s_http_server_opts.enable_directory_listing = "yes";
    
    // Start mongoose thread
    int err = pthread_create(&kh->tid, NULL, &http_thread, kh);
    if (err != 0) printf("Can't create mongoose thread :[%s]\n", strerror(err));
    else printf("Mongoose thread created successfully\n");       
    
    return kh;
}

/**
 * Destroy teonet HTTP module
 * @param kh
 */
void ksnHTTPDestroy(ksnHTTPClass *kh) {
    
    kh->stop = 1;
    while(!kh->stopped) usleep(1000);
    free(kh);
}

/**
 * Teonet event handler
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 * @param user_data
 */
void ksnHTTPEventCb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {

    // Switch Teonet events
    switch(event) {

        case EV_K_ASYNC:
            {
                struct mg_connection *nc = user_data;
                struct teoweb_data *td = data;
                
                if(data != NULL) {
                    
                    switch(td->cmd) {
                        
                        case WS_CONNECTED:
                            printf("Async event was received from %p, "
                                "connected\n", nc);
                            break;
                            
                        case WS_DISCONNECTED:
                            printf("Async event was received from %p, "
                                "disconnected\n", nc);
                            break;
                            
                        case WS_MESSAGE: 
                            printf("Async event was received from %p, "
                                   "%d bytes: '%.*s'\n", 
                                   nc, (int)data_len, (int)td->data_len, 
                                   (char*) td->data);
                            break;
                    }
                }                
            }
            break;
            
        default:
            break;
    }
}
