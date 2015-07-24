/** 
 * File:   teomulti_t.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Connect to and manage some teo-networks in one time (with using threads). 
 * The networks are divided by the host port number.
 * 
 * Created on July 22, 2015, 11:23 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "ev_mgr.h"

#define TMULTI_T_VERSION VERSION

#define TEONET_NUM 3
const int TEONET_PORTS[TEONET_NUM] = { 9301, 9302, 9303 }; // Port numbers
const char *TEONET_NAMES[TEONET_NUM] = { "TEO-A", "TEO-B", "TEO-C" }; // Hosts names

typedef struct teonet_multi {
    
    size_t n_num;
    size_t num_nets;
    int argc; 
    char** argv;
    void (*event_cb)(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data);
    int options;
    int port;
    const char *name;
    pthread_t tid;
    ksnetEvMgrClass *ke;
    ksnetEvMgrClass *n_prev;
    ksnetEvMgrClass *n_next;
    
} teonet_multi;

/**
 * Fossa main thread function
 * 
 * @param arg
 * @return 
 */
void* teonet_t(void *teo) {
    
    #define teonet ((teonet_multi *)teo)
            
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(teonet->argc, teonet->argv, 
            teonet->event_cb, teonet->options, teonet->port, NULL);
    
    // Set network parameters
    ke->n_num = teonet->n_num; // Set network number
    ke->num_nets = teonet->num_nets; // Set number of networks
    strcpy(ke->ksn_cfg.host_name, teonet->name); // Set host name
    ke->n_prev = teonet->n_prev; // Pointer to previous network
    ke->n_next = teonet->n_next; // Pointer to next network
    teonet->ke = ke; // Set pointer to event manager
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    return NULL;
    
    #undef teonet
}

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {

    int i;
    teonet_multi teonet[TEONET_NUM];
    
    printf("Teomulti_t " TMULTI_T_VERSION "\n");
    
    // Start teonet threads
    for(i = 0; i < TEONET_NUM; i++) {
        
        // Set teonet thread parameters
        teonet[i].ke = NULL;
        teonet[i].n_num = i;
        teonet[i].num_nets = TEONET_NUM;
        teonet[i].argc = argc;
        teonet[i].argv = argv;
        teonet[i].event_cb = NULL;
        teonet[i].options = READ_OPTIONS|READ_CONFIGURATION;
        teonet[i].port = TEONET_PORTS[i];
        teonet[i].name = TEONET_NAMES[i];
        if(i) teonet[i].n_prev = teonet[i-1].ke;
        else teonet[i].n_prev = NULL;     
        teonet[i].n_next = NULL;
        
        // Create teonet thread
        int err = pthread_create(&teonet[i].tid, NULL, &teonet_t, &teonet[i]);
        if (err != 0) printf("\nCan't create thread :[%s]", strerror(err));
        else printf("\nThread created successfully\n");
        
        // Wait while teonet started
        while(teonet[i].ke == NULL || teonet[i].ke->runEventMgr != 1) {
            usleep(100);
        }
        
        // Add next net parameter to previous network
        if(i) teonet[i-1].ke->n_next = teonet[i].ke;

//        // Show previous/next 
//        if(i) {
//            int j;
//            for(j = 0; j < 2; j++) {
//                printf("Net #%d started, n_prev: %p, n_next: %p\n", 
//                       (int)teonet[j+i-1].n_num, 
//                       (void *)teonet[j+i-1].n_prev, 
//                       (void *)teonet[j+i-1].n_next);
//                if(!(i == TEONET_NUM - 1)) break;
//            }
//        }
    }
    
    // Wait while threads running
    for(;;) {
        
        int isRunning = 1;
        for(i = 0; i < TEONET_NUM; i++) {
            if(!teonet[i].ke->runEventMgr) {
                isRunning = 0;
                break;
            }
        }
        if(!isRunning) break;
        usleep(100);
    }    
    
    // Stop other threads
    for(i = 0; i < TEONET_NUM; i++) {
        
        ksnetEvMgrStop(teonet[i].ke);
    }
    
    // Wait threads stopped
    usleep(KSNET_EVENT_MGR_TIMER*1000000);
    
    return (EXIT_SUCCESS);
}
