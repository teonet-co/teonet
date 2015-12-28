/* 
 * File:   teoroom.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on December 17, 2015, 1:52 PM
 */

/*
 *
 * Command line to execute:
 * 
 *  app/teoroom teo-room
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pbl.h>

#include "ev_mgr.h"

#define TROOM_VERSION "0.0.1"

// Application API command
enum CMD_R {    
    CMD_R_START = CMD_USER, ///< #129 Start game
    CMD_R_POSITION,         ///< #130 Transfer position
    CMD_R_END               ///< #131 End game 
};

PblMap _map;
PblMap *map;

#define sendCmdTo(ke, rd, name, out_data, out_data_len) \
    if(rd->l0_f) \
    ksnLNullSendToL0(ke, \
        rd->addr, rd->port, name, strlen(name) + 1, rd->cmd, \
        out_data, out_data_len); \
    else \
        ksnCoreSendCmdto(ke->kc, name, rd->cmd, \
            out_data, out_data_len)


static void send_to_all(ksnetEvMgrClass *ke, ksnCorePacketData *rd) {
    
    // Resend data to all users except of me
    PblIterator *it = pblMapIteratorNew(map);
    if(it != NULL) {
        
        
        void *out_data = malloc(rd->data_len + rd->from_len);
        if(rd->data_len) memcpy(out_data, rd->data, rd->data_len);
        memcpy(out_data + rd->data_len, rd->from, rd->from_len);
        size_t out_data_len = rd->data_len + rd->from_len;

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = (char *) pblMapEntryKey(entry);
            
            if(strcmp(rd->from, name)) {
                
                printf("Resend cmd %d to user: %s\n", rd->cmd, name);
                
                // \todo Issue #141: Create teonet command to send data to Peer or to L0 client                
                sendCmdTo(ke, rd, name, out_data, out_data_len);
            }
        }
        
        pblIteratorFree(it);
        free(out_data);
    }
}

/**
 * Send all existing users (R_START) to this user
 * 
 * @param ke
 * @param rd
 */
static void send_all_connected(ksnetEvMgrClass *ke, ksnCorePacketData *rd) {

    // Resend data to all users except of me
    PblIterator *it = pblMapIteratorNew(map);
    if(it != NULL) {
        
        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = (char *) pblMapEntryKey(entry);
            size_t name_len = pblMapEntryKeyLength(entry);
            
            if(strcmp(rd->from, name)) {

                printf("Resend cmd %d about user %s to user: %s\n", rd->cmd, 
                        name, rd->from);
                sendCmdTo(ke, rd, rd->from, name, name_len);
            }
        }
        
        pblIteratorFree(it);
    }
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
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {

    switch(event) {

        // DATA received
        case EV_K_RECEIVED:
        {
            // DATA event
            ksnCorePacketData *rd = data;
            
            printf("Got cmd # %d, from %s\n", rd->cmd, rd->from);
            
            switch(rd->cmd) {
                
                // Server got START from client
                case CMD_R_START:
                {
                    printf("Got START from: %s\n", rd->from); 
                    
                    if(!pblMapContainsKeyStr(map, rd->from)) {
                        
                        // Send all existing users (R_START) to this user
                        send_all_connected(ke, rd);
                        
                        // Add to user map
                        pblMapAdd(map, rd->from, rd->from_len, "", 1);
                        
                        // Resend START to all users
                        send_to_all(ke, rd);
                        
                        // \todo: Subscribe to client disconnected command at L0 server
                    }
                    
                } break;    
                    
                // Server got POSITION from client
                case CMD_R_POSITION:
                {
                    
                    printf("Got POSITION from: %s\n", rd->from); 

                    // \todo If not connected
                    if(!pblMapContainsKeyStr(map, rd->from)) {
                        
                        printf("Emulate START from: %s\n", rd->from); 
                        
                        // Add to user map
                        pblMapAdd(map, rd->from, rd->from_len, "", 1);
                    }
                    
                    // Resend POSITION to all users
                    send_to_all(ke, rd);
                    
                } break;    
                    
                // Server got END from client
                case CMD_R_END:
                {
                    printf("Got END from: %s\n", rd->from); 
                    
                    // Resend END to all user
                    send_to_all(ke, rd);
                    
                    // Remove user from map
                    size_t valueLength;
                    pblMapRemove(map, rd->from, rd->from_len, &valueLength);
                    
                } break;    
                    
                default:
                    break;
            }   
        }
        break;
        
        default:
            break;
    }
}

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teo room ver " TROOM_VERSION ", based on teonet ver " VERSION "\n");
    
    map = pblMapNewHashMap();
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb, READ_ALL);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    pblMapFree(map);

    return (EXIT_SUCCESS);
}

