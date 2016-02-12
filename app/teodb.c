/**
 * File:   teodb.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet database based at PBL KEYFILE:
 * -- C key file, C-KeyFile
 * An open source C key file implementation, see pblKf functions
 *
 * Features:
 *
 *  * ultra fast B* tree implementation for random lookups
 *  * transaction handling
 *  * sequential access methods
 *  * embeddable small footprint, < 35 Kb
 *  * arbitrary size files, up to 4 terrabytes
 *  * arbitrary number of records per file, up to 2 ^ 48 records
 *  * duplicate keys
 *  * advanced key compression for minimal size B trees
 *  * keylength up to 255 bytes
 *  * regression test frame
 *
 * The basic functions (commands) of the application:
 *
 *  * set - set data to namespace by key
 *  * get - get data from namespace by key
 *
 *  whre:
 *
 *  - Namespace is a filename of PBL KEYFILE
 *  - key is a duplicate key in PBL KEYFILE
 *  - data is a data saved in PBL KEYFILE
 *
 * Created on August 20, 2015, 3:36 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "embedded/jsmn/jsmn.h"
#include "ev_mgr.h"

#define TDB_VERSION "0.0.1"
#define APPNAME "\033[22;35m" "Teodb: " "\033[0m"

/**
 * Teonet database API commands
 */
enum CMD_D {

    CMD_D_SET = 129,    ///< #129 Set data request:  type_of_request: { id, namespace, key, data, data_len } }
    CMD_D_GET,          ///< #130 Get data request:  type_of_request: { id, namespace, key } }
    CMD_D_GET_ANSWER,   ///< #131 Get data response: { namespace, key, data, data_len } }

    // Reserved
    CMD_R_NONE          ///< Reserved
};

/**
 * Teo DB binary network structure
 */
typedef struct teo_db_data {
    
    uint8_t key_length;
    uint32_t data_length;
    char key_data[];    
    
} teo_db_data;

/**
 * Check data type and show message
 */
#define get_data_type() rd->data_len && \
                        rd->data_len > JSON_LEN && \
                        !strncmp(rd->data, JSON, JSON_LEN)  ? 1 : 0; \
    \
    ksnet_printf(&ke->ksn_cfg, DEBUG, APPNAME \
        "Got cmd: %d, type: %s, from: %s\n", \
        rd->cmd, data_type  ? JSON : BINARY, rd->from \
    )

/**
 * Convert data to JSON string
 *
 * @param data
 */
#define data_to_json_str(data) \
    (char*)data + JSON_LEN + (((char*)data)[JSON_LEN] == ':' ? 1 : 0)

/**
 * Replace substring in string
 *
 * @param target
 * @param source
 * @param needle
 * @param to
 * @return
 */
static char *str_replace(char *target, char *source, char *needle, char *to) {

    size_t needle_len = strlen(needle);
    size_t to_len = strlen(to);
    char *fnd = NULL;
    size_t ptr = 0;


    if((fnd = strstr(source + ptr, needle)) != NULL) {

        size_t fnd_pos = fnd - (source + ptr);
        strncpy(target, source, fnd_pos); target[fnd_pos] = 0;
        strcat(target, to);
//        strcat(target + fnd_pos + to_len, fnd + needle_len);

      str_replace(target + fnd_pos + to_len, fnd + needle_len, needle, to);
    }
    else strcpy(target, source);

    return target;
}

/**
 * Check json key equalize to string
 * 
 * @param json Json string
 * @param tok Jsmt json token
 * @param s String to compare
 * 
 * @return Return 0 if found
 */
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    
    if (tok->type == JSMN_STRING && 
        (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        
        return 0;
    }
    
    return -1;
}

/**
 * JSON request parameters structure
 * 
 * @member key
 * @member data
 */
typedef struct json_param {
    
    char *key;
    char *data;
    
} json_param;

/**
 * Parse JSON request
 * 
 * @param json_str
 * @param jp
 * @return 
 */
static int json_parse(char *data, json_param *jp) {
    
    // Define json type of data field
    //
//    char *beg, *end;
    jsmntype_t type = JSMN_UNDEFINED;
    size_t data_length = strlen(data);
    size_t num_of_tags = get_num_of_tags(data, data_length);
    if(num_of_tags) {

        // Parse json
        jsmntok_t *t = malloc(num_of_tags * sizeof(jsmntok_t));
        //
        jsmn_parser p;
        jsmn_init(&p);
        int r = jsmn_parse(&p, data, data_length, t, num_of_tags);    
        if(!(r < 1)) type = t[0].type;
        //
        free(t);
    }
    else return 0;
    
//    if(type == JSMN_STRING || type == JSMN_UNDEFINED) beg = end = "\"";
//    else beg = end = "";             
   
    jsmntok_t *t = malloc(num_of_tags * sizeof(jsmntok_t));
        
    // Parse json
    jsmn_parser p;
    jsmn_init(&p);
    int r = jsmn_parse(&p, data, data_length, t, num_of_tags);    
    if(r < 0) {
        
        // This is not JSON string - skip processing
        // printf("Failed to parse JSON: %d\n", r);
        free(t);
        return 0;
    }

    // Assume the top-level element is an object 
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        
        // This is not JSON object - skip processing
        // printf("Object expected\n");
        free(t);
        return 0;
    }

    // Teonet db json string format:
    //
    // {"key":"Test-22","data":"Test data"}
    // or
    // {"key":"Test-22"}
    //
    enum KEYS {
        KEY = 0x1,
        DATA = 0x2, // 0x4 0x8
        ALL_KEYS = KEY | DATA
    };
    const char *KEY_TAG = "key";
    const char *DATA_TAG = "data";
    int i, keys = 0;
    // Loop over json keys of the root object and find needle: cmd, to, cmd_data
    for (i = 1; i < r && keys != ALL_KEYS; i++) {
        
        if(jsoneq(data, &t[i], KEY_TAG) == 0) {
            
            jp->key = strndup((char*)data + t[i+1].start, 
                    t[i+1].end-t[i+1].start);
            keys |= KEY;
            i++;
                
        } else if(jsoneq(data, &t[i], DATA_TAG) == 0) {
            
            jp->data = strndup((char*)data + t[i+1].start, 
                    t[i+1].end-t[i+1].start);
            keys |= DATA;
            i++;
        }
    }
    free(t);
        
    return 1;
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

    // Switch Teonet event
    switch(event) {
        
        // Set default namespace
        case EV_K_STARTED:
            ksnTDBnamespaceSet(ke->kf, "test");
            break;

        case EV_K_STOPPED_BEFORE:
            break;
            
        // DATA received
        case EV_K_RECEIVED:
        {
            // DATA event
            ksnCorePacketData *rd = data;

            // Request type
            const int JSON_LEN = 4;
            const char *JSON = "JSON";
            const char *BINARY = "BINARY";

            switch(rd->cmd) {

                // Set data request
                case CMD_D_SET:
                {
                    // Get type of request JSON - 1 or BINARY - 0
                    const int data_type = get_data_type();
                    
                    // JSON data
                    if(data_type) {

                        char *json_data = data_to_json_str((char*)rd->data);
                        char *json_data_unesc =
                            trim(str_replace(strdup(json_data),
                                json_data, "\\\"", "\""));

                        ksnet_printf(&ke->ksn_cfg, DEBUG, APPNAME
                                "CMD_D_SET data: %s\n", json_data_unesc);

                        // Parse request
                        json_param jp = { NULL, NULL };
                        json_parse(json_data_unesc, &jp);
                        
                        // \todo Process the data
                        int rv = -1;
                        if(jp.key != NULL && jp.data != NULL) {
                            rv = ksnTDBsetStr(ke->kf, jp.key, jp.data, strlen(jp.data) + 1);
                        }
                        if(!rv)
                            ksnet_printf(&ke->ksn_cfg, DEBUG, APPNAME
                                "KEY: %s, DATA: %s, DB set status: %d\n", jp.key, jp.data, rv);
                        else
                           ksnet_printf(&ke->ksn_cfg, DEBUG, APPNAME
                                "An error happened during write to DB, rv: %d ...\n", rv); 

                        // Free data
                        if(jp.key != NULL) free(jp.key);
                        if(jp.data != NULL) free(jp.data);
                        free(json_data_unesc);
                    }
                    
                    // BINARY data
                    else {
                        teo_db_data *tdd = rd->data;
                        
                        int rv = -1;
                        if(tdd->key_length && 
                           tdd->key_length + tdd->data_length == rd->data_len) {
                            
                            rv = ksnTDBset(ke->kf, 
                                tdd->key_data, tdd->key_length, 
                                tdd->key_data + tdd->key_length, tdd->data_length);
                        }
                    }
                }
                break;

                // Get data request
                case CMD_D_GET:
                {
                    // Get type of request JSON - 1 or BINARY - 0
                    const int data_type = get_data_type();
                    
                    // JSON data
                    if(data_type) {

                        char *json_data = data_to_json_str((char*)rd->data);
                        char *json_data_unesc =
                            trim(str_replace(strdup(json_data),
                                json_data, "\\\"", "\""));

                        ksnet_printf(&ke->ksn_cfg, DEBUG, APPNAME
                                "CMD_D_GET data: %s\n", json_data_unesc);

                        // Parse request
                        json_param jp = { NULL, NULL };
                        json_parse(json_data_unesc, &jp);
                        
                        // Process the data
                        size_t data_len;
                        char *data = ksnTDBgetStr(ke->kf, jp.key, &data_len);
                        ksnet_printf(&ke->ksn_cfg, DEBUG, APPNAME
                                "KEY: %s, DATA: %s, DB get status: %d\n", 
                                jp.key, data != NULL ? data : "", data == NULL);
                        
                        // Send ANSWER
                        void *data_out = ksnet_formatMessage(
                            "{ \"key\":\"%s\", \"data\":\"%s\" }",  
                            jp.key, data != NULL ? data : "");
                        size_t data_out_len = strlen(data_out);
                        // Send ANSWER to L0 user
                        if(rd->l0_f)
                            ksnLNullSendToL0(ke, 
                                    rd->addr, rd->port, rd->from, rd->from_len, 
                                    CMD_D_GET_ANSWER, 
                                    data_out, data_out_len);
                        // Send ANSWER to peer
                        else
                            ksnCoreSendto(ke->kc, rd->addr, rd->port, 
                                    CMD_D_GET_ANSWER,
                                    data_out, data_out_len);
                        
                        // Free DB data
                        free(data_out);
                        free(data);

                        // Free data
                        if(jp.key != NULL) free(jp.key);
                        if(jp.data != NULL) free(jp.data);
                        free(json_data_unesc);
                    }
                    
                    // BINARY data
                    else {
                        teo_db_data *tdd = rd->data;
                        
                        if(tdd->key_length && tdd->key_length == rd->data_len) {
                            
                            size_t data_len;
                            char *data = ksnTDBget(ke->kf, tdd->key_data, 
                                    tdd->key_length, &data_len);
                            
                            // Create output data
                            size_t data_out_len = sizeof(teo_db_data) + tdd->key_length + data_len;
                            teo_db_data *data_out = malloc(data_out_len);
                            data_out->key_length = tdd->key_length;
                            data_out->data_length = data_len;
                            memcpy(data_out->key_data, tdd->key_data, tdd->key_length);
                            memcpy(data_out->key_data + tdd->key_length, data, data_len);
                            
                            // Send ANSWER to L0 user
                            if(rd->l0_f)
                                ksnLNullSendToL0(ke, 
                                        rd->addr, rd->port, rd->from, rd->from_len, 
                                        CMD_D_GET_ANSWER, 
                                        data_out, data_out_len);
                            // Send ANSWER to peer
                            else
                                ksnCoreSendto(ke->kc, rd->addr, rd->port, 
                                        CMD_D_GET_ANSWER,
                                        data_out, data_out_len);
                            
                            free(data_out);
                        }
                    }

                }
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

    printf("Teonet database ver " TDB_VERSION ", based on teonet ver "
            "%s" "\n", teoGetLibteonetVersion());

    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb, READ_ALL);

    // Set application type
    teoSetAppType(ke, "teo-db");

    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
