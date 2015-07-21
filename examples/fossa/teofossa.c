/** 
 * File:   teofossa.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Check integration teonet with fossa.
 * The Teonet runs in main thread and the fossa runs in separate thread.
 * 
 * How to execute this test:
 * 
 * 1) Start this test application in terminal:
 * 
 *      tests/fossa/teofossa teofossa -a 127.0.0.1
 * 
 * 2) Start telnet in other terminal:
 * 
 *      telnet 0 1234
 * 
 * 3) Type in the terminal some text:
 * 
 *      Hello world!
 * 
 * 4) Press Ctrl+C in the teonet terminal window to stop this test.
 * 
 * The text "Hello world!" will be:
 * 
 *   - received by fossa in fossa event handler
 *   - resends to teonet with the ksnetEvMgrAsync function
 *   - send back to terminal with fossa ns_send function in teonet event handler
 * 
 * 
 * Created on July 20, 2015, 5:09 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
              size_t data_len, void *user_data) {

    switch (ev) {
        
        case EV_K_ASYNC:
            {
                char *d = strndup(data, data_len); // Create temporary data buffer to add 0 at the end of string
                d[strcspn(d, "\r\n")] = 0; // Remove trailing CRLF
                printf("Async event was received %d bytes: %s\n", (int)data_len, (char*) d);
                
                // This event handler implements simple TCP echo server
                ns_send(user_data, data, data_len);  // Echo received data back
                free(d); // Free temporary data buffer
            }
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
            ksnetEvMgrAsync(nc->mgr->user_data, io->buf, io->len, nc);
                    
            // Discard data from recv buffer
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
        ns_mgr_poll(&mgr, 100);
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
