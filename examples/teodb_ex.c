/* 
 * /file   teodb_ex.c
 * /author Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teodb_ex.c
 *
 * ## Using the Teonet DB function  
 * Module: teodb.c and teodb_com.c  
 *   
 * This is example application to test connection to teo-db application set and 
 * get DB data. 

 * Created on February 15, 2016, 2:11 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"
#include "modules/teodb_com.h"

#define TDB_VERSION "0.0.1"

#define TEODB_PEER ke->ksn_cfg.app_argv[1]

#define TEODB_EX_KEY "teo_db_ex"
#define TEST_KEY "test"
#define TEST_VALUE "Test value"

/**
 * Callback Queue callback (the same as callback queue event). 
 * 
 * This function calls at timeout or after ksnCQueExec calls
 * 
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback 
 * @param data User data
 */
void get_cb(uint32_t id, int type, void *data) {
    
    teo_db_data **tdd = data;
            
    printf("5) Got Callback Queue callback with id: %o, type: %d => %s\n"
           "Key: \"%s\", Value: \"%s\"\n", 
           id, type, type ? "success" : "timeout",
           (*tdd)->key_data, (*tdd)->key_data + (*tdd)->key_length);
    
    printf("Test finished ...\n");
}


/**
 * Teonet Events callback
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param event Teonet Event (ksnetEvMgrEvents)
 * @param data Events data
 * @param data_len Data length
 * @param user_data Some user data (may be set in ksnetEvMgrInitPort())
 */
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {

    // Check teonet event
    switch(event) {
        
        case EV_K_CONNECTED:
        {
            // Client send subscribe command to server                
            char *peer = ((ksnCorePacketData*)data)->from;
            if(!strcmp(peer, TEODB_PEER)) {

                printf("The TeoDB peer: '%s' was connected\n",
                       TEODB_PEER);
                
                const char *key = TEODB_EX_KEY "." TEST_KEY;
                const char *value = TEST_VALUE;
                
                // 1) Add test key to the TeoDB
                printf("1) Add test key to the TeoDB, key: %s, value: %s\n", 
                       key, value);
                
                // Prepare data
                size_t key_len = strlen(key) + 1;
                size_t data_len = strlen(value) + 1;
                size_t tdd_len = sizeof(teo_db_data) + key_len + data_len;
                teo_db_data *tdd = malloc(tdd_len);
                tdd->key_length = key_len;
                tdd->data_length = data_len;
                tdd->id = 0;
                memcpy(tdd->key_data, key, key_len);
                memcpy(tdd->key_data + key_len, value, data_len);
                
                // Send SET command to DB peer
                ksnCoreSendCmdto(ke->kc, TEODB_PEER, CMD_D_SET, tdd, tdd_len);  
                free(tdd);
                
                // 2) Send GET request to the TeoDB
                printf("2) Send GET request to the TeoDB, key: %s\n", 
                           key);                    
                
                // Prepare data
                key_len = strlen(key) + 1;
                data_len = 0;
                tdd_len = sizeof(teo_db_data) + key_len + data_len;
                tdd = malloc(tdd_len);
                tdd->key_length = key_len;
                tdd->data_length = data_len;
                tdd->id = 0;
                memcpy(tdd->key_data, key, key_len);
                memcpy(tdd->key_data + key_len, value, data_len);
                
                // Send GET command to DB peer
                ksnCoreSendCmdto(ke->kc, TEODB_PEER, CMD_D_GET, tdd, tdd_len);  
                free(tdd);
            }
        }
        break;
        
        case EV_K_RECEIVED:
        {
            // DATA event
            ksnCorePacketData *rd = data;

            switch(rd->cmd) {
                
                // Get data request #132
                case CMD_D_GET_ANSWER:
                {
                    static teo_db_data *tdd;
                    tdd = rd->data;
                            
                    char *key = tdd->key_data;
                    char *value = tdd->key_data + tdd->key_length;
                    
                    if(!tdd->id) {
                        
                        // 3) Got test key value from the TeoDB
                        printf("3) Got test key value from the TeoDB, "
                               "key: %s, value: %s\n", 
                               key, value);   

                        // 4) \todo Send GET request to the TeoDB using cQueue
                        printf("4) Send GET request to the TeoDB using cQueue, "
                               "key: %s\n", 
                               key);

                        // Add callback to queue and wait timeout after 5 sec ...
                        ksnCQueData *cq = ksnCQueAdd(ke->kq, get_cb, 5.000, &tdd);
                        printf("4.1) Register callback id %d\n", cq->id);

                        // Prepare data
                        size_t key_len = strlen(key) + 1;
                        size_t data_len = 0;
                        size_t tdd_len = sizeof(teo_db_data) + key_len + data_len;
                        tdd = malloc(tdd_len);
                        tdd->key_length = key_len;
                        tdd->data_length = data_len;
                        tdd->id = cq->id;
                        memcpy(tdd->key_data, key, key_len);
                        memcpy(tdd->key_data + key_len, value, data_len);

                        // Send GET command to DB peer
                        ksnCoreSendCmdto(ke->kc, TEODB_PEER, CMD_D_GET, tdd, tdd_len);  
                        free(tdd);
                    }
                    else {
                        // Execute callback queue to make success result
                        ksnCQueExec(ke->kq, tdd->id);
                    }
                }
                break;
            }
        }
        break;
        
        // Other events
        default: break;
    }
}
        
/**
 * Main Teonet Database example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 * 
 * @return
 */
int main(int argc, char** argv) {

    printf("Teodb example ver " TDB_VERSION ", "
           "based on teonet ver. " VERSION "\n");
    
    // Application parameters
    char *app_argv[] = { "", "teodb_peer"}; 
    ksnetEvMgrAppParam app_param;
    app_param.app_argc = 2;
    app_param.app_argv = app_argv;
    
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInitPort(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);   

    // Set application type
    teoSetAppType(ke, "teo-db-ex");
    
//    // Set custom timer interval. See "case EV_K_TIMER" to continue this example
//    ksnetEvMgrSetCustomTimer(ke, 2.00);
    
    // Show Hello message
    ksnet_printf(&ke->ksn_cfg, MESSAGE, "Example started...\n\n");

    // Start teonet
    ksnetEvMgrRun(ke);    
    
    
    return (EXIT_SUCCESS);
}

