/** 
 * File:   teo_web.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet HTTP/WS Server module
 *
 * Created on October 2, 2015, 12:04 AM
 */

#include "teo_web.h"
#include "teo_ws.h"

/**
 * Teonet websocket class
 */
#define tws ((teoWSClass *)kh->kws)

/**
 * Mongoose websocket send broadcast
 * 
 * @param nc Pointer to structure mg_connection
 * @param msg Message
 * @param len Message length
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
static void teoSendAsync(struct mg_connection *nc, uint16_t cmd, void *data, 
        size_t data_len) {
    
    size_t td_size = sizeof(struct teoweb_data) + data_len;
    struct teoweb_data *td = malloc(td_size);
    td->cmd = cmd; 
    td->data_len = data_len;
    if(data_len) memcpy(td->data, data, data_len);
    ksnetEvMgrAsync(((ksnHTTPClass *)nc->mgr->user_data)->ke, td, td_size, nc);
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
    
    int connect_status;
    struct http_message *hm  = ((struct http_message *) ev_data);
    struct websocket_message *wm = ((struct websocket_message *) ev_data);
    
    ksnHTTPClass *kh = ((ksnHTTPClass *) nc->mgr->user_data);
    
    // Check server events
    switch(ev) {

        // Serve HTTP request
        case MG_EV_HTTP_REQUEST: 
            
            printf("\n### MG_EV_HTTP_REQUEST: \n");            
            printf("HTTP URI: %.*s\n", (int)hm->uri.len, hm->uri.p);
            
            // Connect to other server if needle URI sent
            const char *auth_uri = "/api/auth/";
            const char *auth_server_url = "http://172.17.0.2:1234"; 
            
            if(!strncmp(hm->uri.p, auth_uri, strlen(auth_uri))) {
                
                printf("!!! %s  !!! nc: %p, user_data: %p\n", 
                        auth_uri, nc, nc->user_data);
                
                // Connect to remote HTTP server and resend request(headers)
                const size_t auth_url_len = hm->uri.len + strlen(auth_server_url);
                char auth_url[auth_url_len + 1];
                strcpy(auth_url, auth_server_url);
                memcpy(auth_url + strlen(auth_server_url), hm->uri.p, hm->uri.len);
                auth_url[auth_url_len] = 0;
                
                printf("auth_url: %s\n", auth_url);
                printf("message: \n%.*s\n", (int)hm->message.len, hm->message.p);
                    
//                // Post
//                struct mg_connection *nc_con = mg_connect_http(nc->mgr, 
//                        mg_ev_handler, auth_url, hm->message.p, "");

                // Get
                struct mg_connection *nc_con = mg_connect_http(nc->mgr, 
                        mg_ev_handler, auth_url, NULL, NULL);

                // Save current connection to user_data of new connection
                if(nc_con != NULL) {
                    nc_con->user_data = nc;
                }
            }
            else {
                mg_serve_http(nc, hm, kh->s_http_server_opts);
                nc->flags |= MG_F_SEND_AND_CLOSE;
            }
            break;
            
        // Connected to remote HTTP server    
        case MG_EV_CONNECT:       
            
            connect_status = * (int *) ev_data;
            printf("\n### MG_EV_CONNECT: \n");
            if (connect_status == 0) {
                printf("Connected to nc = %p, nc->user_data: %p ...\n", 
                        nc, nc->user_data);
                //mg_printf(nc, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", "", "");
            } else {
                printf("Error connecting to remote HTTP server: %s\n", 
                      strerror(connect_status));
            }
            break;
            
        // Replay from remote HTTP server
        case MG_EV_HTTP_REPLY:
            
            printf("\n### MG_EV_HTTP_REPLY: \n");
            printf("MG_EV_HTTP_REPLY: nc = %p, nc->user_data: %p\n", 
                    nc, nc->user_data);
            printf("Got reply:\n%.*s\n", (int) hm->message.len, hm->message.p); //   (int) hm->body.len, hm->body.p);
            
            // Send replay to user
            if(nc->user_data != NULL) {
                mg_printf(nc->user_data, "%.*s",  (int) hm->message.len, hm->message.p); //   (int) hm->body.len, hm->body.p);
                ((struct mg_connection *)nc->user_data)->flags |= 
                        MG_F_SEND_AND_CLOSE;
            }
            
            nc->flags |= MG_F_CLOSE_IMMEDIATELY;
            break;
            
        // New websocket connection. Tell everybody. 
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: 
            if(!tws->handler(tws, ev, nc, NULL, 0)) {
                ws_broadcast(nc, "joined", 6); 
                teoSendAsync(nc, WS_CONNECTED, NULL, 0);
            }
            break;
            
        // New websocket message. Tell everybody.
        case MG_EV_WEBSOCKET_FRAME: 
            if(!tws->handler(tws, ev, nc, wm->data, wm->size)) {
                ws_broadcast(nc, (char *) wm->data, wm->size);
                teoSendAsync(nc, WS_MESSAGE, wm->data, wm->size);
            }
            break;
            
        // Disconnect 
        case MG_EV_CLOSE:
            // Disconnect websocket connection. Tell everybody.
            if(is_websocket(nc)) {
                
                if(!tws->handler(tws, ev, nc, NULL, 0)) {
                    ws_broadcast(nc, "left", 4);
                    teoSendAsync(nc, WS_DISCONNECTED, NULL, 0);
                }
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
static void* http_thread(void *kh) {

    struct mg_mgr mgr;
    struct mg_connection *nc;

    mg_mgr_init(&mgr, kh);
    nc = mg_bind(&mgr, ((ksnHTTPClass *)kh)->s_http_port, mg_ev_handler);

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);

    printf("Starting web server on port %s ...\n", 
        ((ksnHTTPClass *)kh)->s_http_port);
    
    for (;;) {
      mg_mgr_poll(&mgr, 2);
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
 * @param tw_cfg Pointer to teoweb_config
 * 
 * @return 
 */
ksnHTTPClass* ksnHTTPInit(ksnetEvMgrClass *ke, teoweb_config *tw_cfg) { 
    
    ksnHTTPClass *kh = malloc(sizeof(ksnHTTPClass));
    kh->ke = ke;
    kh->conf = tw_cfg;
    kh->ta = teoAuthInit(kh); // Initialize authentication class
   
    kh->stop = 0;
    kh->stopped = 0;
    char buffer[KSN_BUFFER_SIZE];
    snprintf(buffer, KSN_BUFFER_SIZE, "%d", (int)tw_cfg->http_port);
    kh->s_http_port = strdup(buffer); 
    memset(&kh->s_http_server_opts, 0, sizeof(kh->s_http_server_opts));
    kh->s_http_server_opts.document_root = tw_cfg->document_root;  // Serve current directory
    kh->s_http_server_opts.enable_directory_listing = "yes";
    kh->s_http_server_opts.index_files = "index.html";
    
    kh->kws = teoWSInit(kh); // Initialize websocket class

    // Start mongoose thread
    int err = pthread_create(&kh->tid, NULL, &http_thread, kh);
    if (err != 0) printf("Can't create mongoose thread :[%s]\n", strerror(err));
    else printf("Mongoose thread created successfully\n");       
    
    return kh;
}

/**
 * Destroy teonet HTTP module
 * 
 * @param kh Pointer to ksnHTTPClass
 */
void ksnHTTPDestroy(ksnHTTPClass *kh) {
        
    kh->stop = 1;
    while(!kh->stopped) usleep(100000);
    tws->destroy(tws);
    free(kh->s_http_port);
    
    teoAuthDestroy(kh->ta);
    
    free(kh);
}
