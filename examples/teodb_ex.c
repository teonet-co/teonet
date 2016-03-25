/** 
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
#define APPNAME _ANSI_MAGENTA "teodb_ex" _ANSI_NONE

#define TEODB_PEER ke->ksn_cfg.app_argv[1]

#define TEODB_EX_KEY "teo_db_ex"
#define TEST_KEY "test"
#define TEST_VALUE "{ \"name\": \"1\" }"

typedef struct get_cq_data {
    
    ksnetEvMgrClass *ke;
    teo_db_data **tdd;
    
} get_cq_data;

/**
 * Callback Queue callback (the same as callback queue event). 
 * 
 * This function calls at timeout or after ksnCQueExec calls
 * 
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback 
 * @param data User data
 */
void list_cb(uint32_t id, int type, void *data) {
    
    get_cq_data *cqd = data;
    #define ke cqd->ke
    #define NUM_RECORDS_TO_SHOW "25"

    void *ar_data = (*cqd->tdd)->key_data + (*cqd->tdd)->key_length;
    uint32_t ar_data_num = *(uint32_t*)ar_data;
    size_t ptr = sizeof(ar_data_num);
    
    printf("7) Got Callback Queue callback with id: %o, type: %d => %s\n"
           "Key: \"%s\"\n", 
           id, type, type ? "success" : "timeout",
           (*cqd->tdd)->key_data);
    
    printf("Number of records in list: %o\n", ar_data_num);
    int i = 0; for(i = 0; i < ar_data_num; i++) {
        size_t len = strlen((char*)ar_data + ptr) + 1;
        printf("%d %s\n", i+1, (char*)ar_data + ptr);
        ptr += len;
        if(i >= atoi(NUM_RECORDS_TO_SHOW) - 1) {
            puts("(the first " NUM_RECORDS_TO_SHOW " entries shows)");
            break;
        }
    }
    puts("");
    
    printf("Test finished ...\n");
    
    free(cqd);
    
    #undef NUM_RECORDS_TO_SHOW
    #undef ke
}

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
    
    get_cq_data *cqd = data;
    #define ke cqd->ke
            
    printf("5) Got Callback Queue callback with id: %o, type: %d => %s\n"
           "Key: \"%s\", Value: \"%s\"\n", 
           id, type, type ? "success" : "timeout",
           (*cqd->tdd)->key_data, (*cqd->tdd)->key_data + (*cqd->tdd)->key_length);
    
    // 6) Send LIST request to the TeoDB using cQueue
    printf("6) Send LIST request to the TeoDB using cQueue\n");

    // Add callback to queue and wait timeout after 5 sec ...    
    ksnCQueData *cq = ksnCQueAdd(ke->kq, list_cb, 5.000, memdup(cqd, sizeof(*cqd)));
    printf("6.1) Register callback id %o\n", cq->id);

    // Prepare data
    size_t tdd_len;
    teo_db_data *tdd = prepare_request_data("", 1, NULL, 0, cq->id, &tdd_len);

    // Send GET command to DB peer
    ksnCoreSendCmdto(ke->kc, TEODB_PEER, CMD_D_LIST, tdd, tdd_len);  
    free(tdd);
    
//    printf("Test finished ...\n");
    
    free(cqd);
    
    #undef ke
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

                printf("The TeoDB peer: \"%s\" was connected\n",
                       TEODB_PEER);
                
                const char *key = TEODB_EX_KEY "." TEST_KEY;
                const char *value = TEST_VALUE;
                
                // 1) Add test key to the TeoDB
                printf("1) Add test key to the TeoDB, key: \"%s\", value: \"%s\"\n", 
                       key, value);
                
                // Prepare data
                size_t tdd_len;
                size_t key_len = strlen(key) + 1;
                teo_db_data *tdd = prepare_request_data(key, key_len, value, strlen(value) + 1, 0, &tdd_len);
                
                // Send SET command to DB peer
                ksnCoreSendCmdto(ke->kc, TEODB_PEER, CMD_D_SET, tdd, tdd_len);  
                free(tdd);
                
                // 2) Send GET request to the TeoDB
                printf("2) Send GET request to the TeoDB, key: \"%s\"\n", 
                           key);                    
                
                // Prepare data
                tdd = prepare_request_data(key, key_len, NULL, 0, 0, &tdd_len);
                
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
            static teo_db_data *tdd;

            switch(rd->cmd) {
                
                // Get data response #132
                case CMD_D_GET_ANSWER:
                {
                    tdd = rd->data;
                            
                    char *key = tdd->key_data;
                    char *value = tdd->key_data + tdd->key_length;
                    
                    if(!tdd->id) {
                        
                        // 3) Got test key value from the TeoDB
                        printf("3) Got test key value from the TeoDB, "
                               "key: \"%s\", value: \"%s\"\n", 
                               key, value);   

                        // 4) \todo Send GET request to the TeoDB using cQueue
                        printf("4) Send GET request to the TeoDB using cQueue, "
                               "key: \"%s\"\n", 
                               key);

                        // Add callback to queue and wait timeout after 5 sec ...
                        get_cq_data *cqd = malloc(sizeof(get_cq_data));
                        cqd->ke = ke;
                        cqd->tdd = &tdd;
                        ksnCQueData *cq = ksnCQueAdd(ke->kq, get_cb, 5.000, cqd);
                        printf("4.1) Register callback id %d\n", cq->id);

                        // Prepare data
                        size_t tdd_len;
                        tdd = prepare_request_data(key, tdd->key_length, NULL, 0, cq->id, &tdd_len);

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
                
                // Get list response #133
                case CMD_D_LIST_ANSWER:
                {
                    // Parse response
                    tdd = rd->data;
                    
                    if(tdd->id) {
                        
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
    ksn_puts(ke, APPNAME, MESSAGE, "started ...\n");

    // Start teonet
    ksnetEvMgrRun(ke);    
    
    
    return (EXIT_SUCCESS);
}

