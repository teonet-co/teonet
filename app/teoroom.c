/** 
 * \file   teoroom.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Room controller
 * 
 * Teonet peer name: teo-room
 * 
 * API, teonet commands:
 * 
 * * CMD_R_START = 129 - Start game, data: GameParameters
 * * CMD_R_POSITION = 130 - Transfer position, data: * (resend to room members)
 * * CMD_R_END = 131 - End game, data: NULL 
 * 
 * Command line to execute:
 * 
 *     app/teoroom teo-room
 * 
 * Created on December 17, 2015, 1:52 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pbl.h>

#include "ev_mgr.h"

#define TROOM_VERSION "0.0.3"

/**
 * Application API commands
 */
enum CMD_R {    
    
    CMD_R_START = CMD_USER, ///< #129 Start game, data: GameParameters
    CMD_R_POSITION,         ///< #130 Transfer position, data: * (resend to room members)
    CMD_R_END               ///< #131 End game, data: NULL 
            
};

/**
 * Application keyboard states
 */
enum {
    
    STATE_NONE,
    STATE_WAIT_KEY,
    STATE_WAIT_STRING
            
} state;

/**
 * Data structure of CMD_R_START command
 */
typedef struct GameParameters {

 uint32_t roomId; ///< Room ID
 
} GameParameters;

/**
 * Room controller (this application) data structure
 */
typedef struct roomClass {
    
    ksnetEvMgrClass *ke; ///< Pointer to ksnetEvMgrClass
    PblMap *map_n; ///< map with client_name key
    PblMap *map_r; ///< map with roomId key
    
    int show_printf; ///< Show terminal messages flag
    int app_state; ///< Application keyboard state
    
} roomClass;

/**
 * Room controllers name map data
 */
typedef struct mapNameData {
    
    uint32_t roomId; ///< Room ID
    char *name; ///< Pointer to client name
    
} mapNameData;

/**
 * Room controllers room map data
 */
typedef struct mapRoomData {
    
    PblSet *set; ///< Room set of pointers to name
    
} mapRoomData;

/**
 * Initialize room module
 * 
 * @return 
 */
static roomClass *roomInit(ksnetEvMgrClass *ke) {
    
    roomClass *rd = malloc(sizeof(roomClass));
    rd->map_n = pblMapNewHashMap();
    rd->map_r = pblMapNewHashMap();
    rd->app_state = STATE_NONE;
    rd->show_printf = 1;
    rd->ke = ke;
    
    return rd;
}

static void roomDestroy(roomClass *rd) {
    
    if(rd != NULL) {
        
        // \todo Remove all clients from name map
        
        pblMapFree(rd->map_n);        
        pblMapFree(rd->map_r);
        
        free(rd);
    }
}


#define sendCmdTo(ke, rd, name, out_data, out_data_len) \
    if(rd->l0_f) \
    ksnLNullSendToL0(ke, \
        rd->addr, rd->port, name, strlen(name) + 1, rd->cmd, \
        out_data, out_data_len); \
    else \
        ksnCoreSendCmdto(ke->kc, name, rd->cmd, \
            out_data, out_data_len)

/**
 * Send data to all clients in this room (in room of rd->from client)
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @param rd Pointer to ksnCorePacketData
 */
static void roomSendToAll(roomClass *room, ksnCorePacketData *rd) {
    
    // Get room of data sender
    size_t valueLength;
    mapNameData *this_md = (mapNameData *)pblMapGet(room->map_n, rd->from, 
            rd->from_len, &valueLength);
    
    // Resend data to all users except of me
    PblIterator *it = pblMapIteratorNew(room->map_n);
    if(it != NULL) {        
        
        void *out_data = malloc(rd->data_len + rd->from_len);
        if(rd->data_len) memcpy(out_data, rd->data, rd->data_len);
        memcpy(out_data + rd->data_len, rd->from, rd->from_len);
        size_t out_data_len = rd->data_len + rd->from_len;

        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = (char *) pblMapEntryKey(entry);
            mapNameData *md = (mapNameData *) pblMapEntryValue(entry);
            
            if(this_md->roomId == md->roomId && strcmp(rd->from, name)) {
                
                if(room->show_printf)
                    printf("Resend cmd %d to user: %s\n", rd->cmd, name);
                
                // \todo Issue #141: Create teonet command to send data to Peer or to L0 client                
                sendCmdTo(room->ke, rd, name, out_data, out_data_len);
            }
        }        
        pblIteratorFree(it);
        
        free(out_data);
    }
}

/**
 * Add client to room maps
 * 
 * @param room Pointer to roomClass
 * @param rd Pointer to ksnCorePacketData
 * @param roomId Room ID
 * 
 * @return int rc >  0: The map did not alrea|dy contain a mapping for the key.
 * @return int rc == 0: The map did already contain a mapping for the key.
 * @return int rc <  0: An error, see pbl_errno:
 */
int roomAddClient(roomClass *room, ksnCorePacketData *rd, uint32_t roomId) {
    
    int retval = -1;
    
    // Add to name map
    mapNameData md;
    md.roomId = roomId;
    md.name = strdup(rd->from);

    retval = pblMapAdd(room->map_n, rd->from, rd->from_len, &md, sizeof(md));
    
    // Add to room map
    size_t valueLength;
    PblSet *set;
    PblSet **set_ptr = pblMapGet(room->map_r, &md.roomId, sizeof(md.roomId), 
            &valueLength);
    if(set_ptr == NULL) {
        
        set = pblSetNewHashSet();
        pblMapAdd(room->map_r, &md.roomId, sizeof(md.roomId), &set, sizeof(set));
        
        if(room->show_printf)
            printf("Room id %d was opened\n", md.roomId);
    }    
    else 
        set = *set_ptr;
    
    // Add pointer to client to room set
    pblSetAdd(set, md.name);
    
    return retval;
}

/**
 * Send END command to all connected clients and remove client from map
 * 
 * @param ke Pointer to ksnetEvMgrClass
 * @param rd Pointer to ksnCorePacketData
 */
static void roomRemoveClient(roomClass *room, ksnCorePacketData *rd) {

    size_t valueLength;
    mapNameData *md;
    if((md = (mapNameData *)pblMapGet(room->map_n, rd->from, rd->from_len, 
            &valueLength)) != NULL) {
        
        // Send to all clients in this room
        roomSendToAll(room, rd);

        // Remove client from room map
        PblSet **set = pblMapGet(room->map_r, &md->roomId, sizeof(md->roomId), 
                &valueLength);
        if(set != NULL) {
            
            // Remove client from room set
            pblSetRemoveElement(*set, md->name);
            
            // Remove room if it is empty
            if(pblSetIsEmpty(*set)) {
                
                pblSetFree(*set);
                pblMapRemoveFree(room->map_r, &md->roomId, sizeof(md->roomId), 
                        &valueLength);
                
                if(room->show_printf)
                    printf("Room id %d was closed\n", md->roomId);
            }
        }
        
        // Remove client from name map
        free(md->name);
        pblMapRemoveFree(room->map_n, rd->from, rd->from_len, &valueLength);    
        
        if(room->show_printf)
            printf("Client %s was removed\n", rd->from);
    }
}

/**
 * Send all existing clients in this room (R_START) to this user
 * 
 * @param ke
 * @param rd
 */
static void roomSendAllConnected(roomClass *room, ksnCorePacketData *rd) {

    // Get this room
    size_t valueLength;
    mapNameData *this_md = (mapNameData *)pblMapGet(room->map_n, rd->from, 
            rd->from_len, &valueLength);
    
    // Resend data to all users except of me
    PblIterator *it = pblMapIteratorNew(room->map_n);
    if(it != NULL) {
        
        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            char *name = (char *) pblMapEntryKey(entry);
            size_t name_len = pblMapEntryKeyLength(entry);
            mapNameData *md = (mapNameData *) pblMapEntryValue(entry);
            
            if(this_md->roomId == md->roomId && strcmp(rd->from, name)) {

                if(room->show_printf)
                    printf("Resend cmd %d of client %s to user: %s\n", rd->cmd, 
                        name, rd->from);
                
                sendCmdTo(room->ke, rd, rd->from, name, name_len);
            }
            
        }
        
        pblIteratorFree(it);
    }
}

/**
 * Print rooms and its clients
 * 
 * @param room Pointer to roomClass
 */
static void roomShowRooms(roomClass *room) {
    
    PblIterator *it = pblMapIteratorNew(room->map_r);
    if(it != NULL) {        
        
        int r = 0;
        const char *line = "---------------\n";
        
        while(pblIteratorHasNext(it)) {

            void *entry = pblIteratorNext(it);
            uint32_t *roomId = (uint32_t *) pblMapEntryKey(entry);
            PblSet **set = (PblSet **) pblMapEntryValue(entry);
            
            if(r > 0) printf("\n");
            printf("Room ID: %d\n", *roomId);
            printf(line);
            int i, num = pblSetSize(*set);
            for(i = 0; i < num; i++) {
                printf("Client: %s\n", (char*)pblSetGet(*set, i));
            }
            printf(line);
            r++;
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

    roomClass *room = (roomClass *) ke->user_data;
            
    switch(event) {

        // Connected event received: Teonet peer was connected
        case EV_K_CONNECTED: 
        {
            char *peer = ((ksnCorePacketData*)data)->from;
            if(!strcmp(peer, "teo-web")) {
                
                if(room->show_printf)
                    printf("L0 server: '%s' was connected\n", peer);
                
                // Subscribe to client disconnected command at L0 server
                teoSScrSubscribe(ke->kc->kco->ksscr, peer, EV_K_L0_DISCONNECTED);
                //teoSScrSubscribe(ke->kc->kco->ksscr, peer, EV_K_L0_CONNECTED);
            }
            
        } break;
        
        // Subscribe event received from subscribe provider
        case EV_K_SUBSCRIBE:
        {
            ksnCorePacketData *rd = data;
            teoSScrData *ssrc_data = rd->data;
            
            if(room->show_printf)
                printf("EV_K_SUBSCRIBE received from: %s, event: %d, name %s\n", 
                    rd->from, ssrc_data->ev, ssrc_data->data);
            
            // Event EV_K_L0_DISCONNECTED from L0 server
            if(ssrc_data->ev == EV_K_L0_DISCONNECTED) {
                                
                ksnCorePacketData rd_s;
                
                // Send event to all clients and remove from map
                rd_s.cmd = CMD_R_END;
                rd_s.from = ssrc_data->data;
                rd_s.from_len = strlen(ssrc_data->data) + 1;
                rd_s.addr = rd->addr, 
                rd_s.port = rd->port,
                rd_s.l0_f = 1,
                rd_s.data = NULL;
                rd_s.data_len = 0;                
                roomRemoveClient(room, &rd_s);
            }
            
        } break;
       

        // DATA received
        case EV_K_RECEIVED:
        {
            // DATA event
            ksnCorePacketData *rd = data;
            
            if(room->show_printf)
                printf("Got cmd # %d, from %s\n", rd->cmd, rd->from);
            
            switch(rd->cmd) {
                
                // Server got START from client
                case CMD_R_START:
                {
                    GameParameters *gp = (GameParameters *)rd->data;
                    
                    //if(room->show_printf)
                    printf("Got START from: %s, roomID: %d (data length: %d)\n", rd->from, 
                            gp->roomId, rd->data_len); 
                    
                    if(!pblMapContainsKeyStr(room->map_n, rd->from)) {
                                           
                        // Add to user map                        
                        roomAddClient(room, rd, gp != NULL ? gp->roomId : 1);
                        
                        // Send all existing users (R_START) to this user
                        roomSendAllConnected(room, rd);
                        
                        // Resend START to all room users
                        roomSendToAll(room, rd);
                    }
                    
                } break;    
                    
                // Server got POSITION from client
                case CMD_R_POSITION:
                {
                    if(room->show_printf)
                        printf("Got POSITION from: %s\n", rd->from); 

                    // If not connected
                    if(!pblMapContainsKeyStr(room->map_n, rd->from)) {
                        
                        //if(room->show_printf)
                        printf("Emulate START from: %s\n", rd->from); 
                        
                        // Add to user map
                        roomAddClient(room, rd, 1);    
                    }
                    
                    // Resend POSITION to all users
                    roomSendToAll(room, rd);
                    
                } break;    
                    
                // Server got END from client
                case CMD_R_END:
                {
                    //if(room->show_printf)
                    printf("Got END from: %s\n", rd->from); 
                    
                    // Resend event to all clients and remove from map
                    roomRemoveClient(room, rd);
                                                
                } break;    
                    
                default:
                    break;
            }   
        }
        break;
        
        // 'A' key was pressed in application hotkey monitor
        case EV_K_USER:
        {
            room->show_printf = 0;
            
            printf ("\n"
                    "Room controller menu:\n"
                    "\n"
                    "  1 - continue show log\n"
                    "  2 - show rooms\n"
                    "  0 - exit application\n"
                    "\n"
                    "> " 
            );
            fflush(stdout);
            room->app_state = STATE_WAIT_KEY;
        }
        break;
            
        // Application hotkey pressed
        case EV_K_HOTKEY:
        {
            if(room->app_state == STATE_WAIT_KEY) {
                
                int command = *(int*)data;
                
                //room->app_state = STATE_NONE;
                switch(command) {

                    case '1':
                        
                        room->show_printf = 1;
                        room->app_state = STATE_NONE;
                        break;
                    
                    case '2':
                        roomShowRooms(room);
                        break;
                        
                    case '0':
                        room->show_printf = 1;
                        room->app_state = STATE_NONE;
                        ksnetEvMgrStop(ke);
                        break;
                        
                    default:
                        printf ("Wrong command %c\n", command);
                        break;
                }
                
                printf ("> ");
                fflush(stdout);
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
    
    roomClass *rd = roomInit(NULL);
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb, READ_ALL, 0, rd);    
    rd->ke = ke;
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    roomDestroy(rd);

    return (EXIT_SUCCESS);
}
