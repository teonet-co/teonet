/* 
 * File:   teofossa.c
 * Author: kirill
 *
 * Created on July 20, 2015, 5:09 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "fossa.h"
#include "ev_mgr.h"

#define TFOSSA_VERSION VERSION

pthread_t tid; // Fossa thread id

/**
 * Teonet event handler
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents ev, void *data,
              size_t data_len) {

    switch (ev) {
        
        case EV_K_ASYNC:
            
            printf("Async event was received\n");
            break;
            
        default:
            break;
    }
}

/**
 * Fossa event handler
 * 
 * @param nc
 * @param ev
 * @param ev_data
 */
static void ev_handler(struct ns_connection *nc, int ev, void *ev_data) {
    
    struct mbuf *io = &nc->recv_mbuf;

    switch (ev) {
        
        case NS_RECV:
            
            // Send event to teonet
            ksnetEvMgrAsync(nc->mgr->user_data, io->buf, io->len);
                    
            // This event handler implements simple TCP echo server
            ns_send(nc, io->buf, io->len);  // Echo received data back
            mbuf_remove(io, io->len);      // Discard data from recv buffer
            break;
            
        default:
            break;
    }
}

/**
 * Fossa main thread function
 * 
 * @param arg
 * @return 
 */
void* fossa(void *ke) {
    
    struct ns_mgr mgr;

    ns_mgr_init(&mgr, ke);  

    // Note that many connections can be added to a single event manager
    // Connections can be created at any point, e.g. in event handler function
    ns_bind(&mgr, "1234", ev_handler);  

    for (;;) {  
        ns_mgr_poll(&mgr, 1000);
    }

    ns_mgr_free(&mgr);
    
    return NULL;
}

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    
    // Hello message
    printf("Teofossa " TFOSSA_VERSION "\n");
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION);

    // Start fossa thread
    int err = pthread_create(&tid, NULL, &fossa, ke);
    if (err != 0) printf("\nCan't create thread :[%s]", strerror(err));
    else printf("\nThread created successfully\n");
    
    // Set custom timer interval
    //ksnetEvMgrSetCustomTimer(ke, 1.00);

    // Hello message
    ksnet_printf(&ke->ksn_cfg, MESSAGE, "Started...\n\n");

    // Start teonet
    ksnetEvMgrRun(ke);
    
    return (EXIT_SUCCESS);
}
