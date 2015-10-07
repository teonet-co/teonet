/**
 * \file   teostream.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teostream.c
 *
 * Teonet stream example
 * 
 * Start stream server:
 *   ```examples/teostream teo-str NULL NULL -p 9000 -r 9099 -a 127.0.0.1```
 * 
 * Start stream client:
 *   ```examples/teostream teostream teo-str str -r 9000 -a 127.0.0.1```
 * 
 * Created on October 5, 2015, 5:06 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ev_mgr.h"

#define TSTR_VERSION "0.0.1"

/**
 * Teonet Events callback
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

        // Calls immediately after event manager starts
        case EV_K_STARTED:
            break;
        
        // This event send when new peer connected to this host (and to the mesh)
        case EV_K_CONNECTED:   
        {
            if(data != NULL) {
                
                if(!strcmp(((ksnCorePacketData*)data)->from, ke->ksn_cfg.app_argv[1])) {
                    
                    printf("Peer \"%s\" was connected\n", ((ksnCorePacketData*)data)->from);

                    printf("Create stream name \"%s\" with peer \"%s\" ...\n",  
                        ke->ksn_cfg.app_argv[2], ke->ksn_cfg.app_argv[1]); 

                    // Send create stream request
                    ksnStreamCreate(ke->ks, ke->ksn_cfg.app_argv[1], 
                        ke->ksn_cfg.app_argv[2],  CMD_ST_CREATE);
                }
            
            }
        } break;
            
        // This event send when stream is connected to this host 
        // WRITE to stream
        case EV_K_STREAM_CONNECTED:    
        {    
            // Get stream parameters from event data (Stream Key)
            ksnStreamData sd;
            ksnStreamMapData *smd = ksnStreamGetMapData(ke->ks, data, data_len);
            ksnStreamGetDataFromMap(&sd, (ksnStreamMapData *)smd);
            
            printf("Stream name \"%s\" is connected to peer \"%s\" ...\n",  
                        sd.stream_name, sd.peer_name); 
            
            // Write data to output FD
            if(write(sd.fd_out, "Hello stream!", 14) >= 0);
            
        } break;
        
        // Stream connection timeout
        case EV_K_STREAM_CONNECT_TIMEOUT:
        {    
            // Get stream parameters from event data (Stream Map Data)
            ksnStreamData sd;
            ksnStreamGetDataFromMap(&sd, (ksnStreamMapData *)data);
            
            // Do something ...
            printf("Can't connect \"%s\" stream to peer \"%s\" during timeout\n",
                    sd.stream_name, sd.peer_name);
            
        } break;
            
        // Got data from connected stream
        // READ from stream
        case EV_K_STREAM_DATA:
        {
            // Get stream parameters from event data (Stream Map Data)
            ksnStreamData sd;
            ksnStreamGetDataFromMap(&sd, (ksnStreamMapData *)data);
            
            // Read data to buffer
            size_t rc;
            char read_buf[KSN_BUFFER_SM_SIZE];
            if((rc = read(sd.fd_in, read_buf, KSN_BUFFER_SM_SIZE)) >= 0) {
                
                printf("Read %d bytes from stream name \"%s\" of peer \"%s\": "
                       "\"%s\"\n", 
                       (int) rc, sd.stream_name, sd.peer_name, read_buf);
            }

        } break;
        
        case EV_K_STREAM_DISCONNECTED:
        {
            // Get stream parameters from event data (Stream Key)
            ksnStreamData sd;
            ksnStreamMapData *smd = ksnStreamGetMapData(ke->ks, data, data_len);
            ksnStreamGetDataFromMap(&sd, (ksnStreamMapData *)smd);
            
            printf("Stream name \"%s\" was disconnected from peer \"%s\" ...\n",  
                        sd.stream_name, sd.peer_name); 
            
        } break;

        // Undefined event (an error)
        default:
            break;
    }
}

/**
 * Main Teostream application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 * 
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teostream example ver " TSTR_VERSION ", based on teonet ver. " VERSION "\n");
    
    // Application parameters
    char *app_argv[] = { "", "peer_to", "stream_name"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 3;
    app_param.app_argv = app_argv;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);
    
    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
