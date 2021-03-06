/**
 * \file   app/teodb.c
 * \author Kirill Scherba <kirill@scherba.ru>
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
 * API:
 *
 *    CMD_D_SET = 129,              ///< #129 Set data request:     { key, data, data_len,  id } }
 *    CMD_D_GET,                    ///< #130 Get data request:     { key, id } }
 *    CMD_D_LIST,                   ///< #131 List request:         { key, id } }
 *    CMD_D_GET_ANSWER,             ///< #132 Get data response:    { key, data, data_len, id } }
 *    CMD_D_LIST_ANSWER,            ///< #133 List response:        [ { key, id }, ... ]
 * 
 *    CMD_D_LIST_LENGTH,            ///< #134 List length request:  { key, id } }
 *    CMD_D_LIST_LENGTH_ANSWER,     ///< #135 List response:        { listLength, key, id }
 * 
 *    CMD_D_LIST_RANGE,             ///< #136 List length request:  { id, key, from, to } }
 *                                  ///< bynary data structure: 
 *                                        typedef struct teo_db_data_range {   
 *                                            uint32_t from; ///< From index (begin from zero)
 *                                            uint32_t to;   ///< To index (not include))
 *                                        };
 *    CMD_D_LIST_RANGE_ANSWER,      ///< #137 List response:        { listLength, key, ID }
 * 
 *
 * Subscribe:
 *
 *  EV_D_SET - #26 send event when database updated
 *
 * Created on August 20, 2015, 3:36 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "embedded/jsmn/jsmn.h"
#include "modules/teodb_com.h"
#include "ev_mgr.h"

#define TDB_VERSION "0.0.6"
#define APPNAME _ANSI_MAGENTA "teodb" _ANSI_NONE

// Constants
#define DEFAULT_NAMESPACE "test"
#define BINARY "BINARY"
#define JSON "JSON"
#define JSON_LEN 4

/**
 * JSON request parameters structure
 *
 * @member key
 * @member data
 */
typedef struct json_param {

    char *key;
    char *data;
    char *id;
    char *from;
    char *to;

} json_param;

/**
 * Check data type and show message
 */
#define get_data_type() rd->data_len && \
                        rd->data_len > JSON_LEN && \
                        !strncmp(rd->data, JSON, JSON_LEN)  ? 1 : 0; \
    \
    ksn_printf(ke, APPNAME, DEBUG,  \
        "Got cmd: %d, type: %s, from: %s\n", \
        rd->cmd, data_type  ? JSON : BINARY, rd->from \
    )

/**
 * Replace substring in string
 *
 * @param target Target string
 * @param source Source string
 * @param needle Substring to find
 * @param to Substring to replace to
 *
 * @return New string with replaced substring, should be free after use
 */
static char *str_replace(char *target, char *source, char *needle, char *to) {

    size_t needle_len = strlen(needle);
    size_t to_len = strlen(to);
    char *fnd = NULL;

    // Find first substring in source string
    if((fnd = strstr(source, needle)) != NULL) {

        // Replace first substring in source string
        size_t fnd_pos = fnd - source;
        strncpy(target, source, fnd_pos); target[fnd_pos] = 0;
        strcat(target, to);

        // Replace next substring in string
        str_replace(target + fnd_pos + to_len, fnd + needle_len, needle, to);
    }
    // Copy source string to target if substring not found
    else strcpy(target, source);

    return target;
}

/**
 * Convert data to JSON string
 *
 * @param data
 * @param data_len
 *
 * @return String contain fixed and unescaped JSON, should be free after use
 */
static char *data_to_json_str(void *data, size_t data_len) {

    char *json_data_unesc_;
    size_t ptr = JSON_LEN + (((char*)data)[JSON_LEN] == ':' ? 1 : 0);
    char *json_data_unesc = strndup((char*)data + ptr, data_len - ptr);

    trim(json_data_unesc);

    json_data_unesc_ = strdup(json_data_unesc);
    str_replace(json_data_unesc, json_data_unesc_, "\\\"", "\"");
    free(json_data_unesc_);

    json_data_unesc_ = strdup(json_data_unesc);
    str_replace(json_data_unesc, json_data_unesc_, "\\\\", "\\");
    free(json_data_unesc_);


    return json_data_unesc;
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
 * Parse JSON request
 *
 * @param json_str
 * @param jp
 *
 * @return Parsed if true, or 0 if error
 */
static int json_parse(char *data, json_param *jp) {

    size_t data_length = strlen(data);
    size_t num_of_tags = get_num_of_tags(data, data_length);

    // Get gags buffer
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
        KEY  = 0x1,
        DATA = 0x2, 
        ID   = 0x4, 
        FROM = 0x8,
        TO   = 0x10,
        ALL_KEYS = KEY | DATA | ID | FROM | TO
    };
    const char *ID_TAG = "id";
    const char *KEY_TAG = "key";
    const char *DATA_TAG = "data";
    const char *FROM_TAG = "from";
    const char *TO_TAG = "to";
    memset(jp, 0, sizeof(*jp)); // Set JSON parameters to NULL
    int i, keys = 0;
    // Loop over json keys of the root object and find needle: cmd, to, cmd_data
    for (i = 1; i < r && keys != ALL_KEYS; i++) {

        // Find KEY tag
        if(!(keys & KEY) && jsoneq(data, &t[i], KEY_TAG) == 0) {

            jp->key = strndup((char*)data + t[i+1].start,
                    t[i+1].end-t[i+1].start);
            keys |= KEY;
            i++;

        }

        // Find DATA tag
        else if(!(keys & DATA) && jsoneq(data, &t[i], DATA_TAG) == 0) {

            jp->data = strndup((char*)data + t[i+1].start,
                    t[i+1].end-t[i+1].start);
            keys |= DATA;
            i++;
        }

        // Find ID tag
        else if(!(keys & ID) && jsoneq(data, &t[i], ID_TAG) == 0) {

            jp->id = strndup((char*)data + t[i+1].start,
                    t[i+1].end-t[i+1].start);
            keys |= ID;
            i++;
        }

        // Find FROM tag
        else if(!(keys & FROM) && jsoneq(data, &t[i], FROM_TAG) == 0) {

            jp->from = strndup((char*)data + t[i+1].start,
                    t[i+1].end-t[i+1].start);
            keys |= FROM;
            i++;
        }

        // Find TO tag
        else if(!(keys & TO) && jsoneq(data, &t[i], TO_TAG) == 0) {

            jp->to = strndup((char*)data + t[i+1].start,
                    t[i+1].end-t[i+1].start);
            keys |= TO;
            i++;
        }
    }
    free(t);

    return 1;
}

/**
 * Free json parameters data
 * 
 * @param jp
 */
static void json_free(json_param *jp) {
    
    if(jp->id != NULL) free(jp->id);
    if(jp->key != NULL) free(jp->key);
    if(jp->data != NULL) free(jp->data);
    if(jp->from != NULL) free(jp->from);
    if(jp->to != NULL) free(jp->to);
}


/**
 * Send answer command
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param rd Pointer to ksnCorePacketData
 * @param data_out Answer data
 * @param data_out_len Answer data length
 */
static void send_answer(ksnetEvMgrClass *ke, ksnCorePacketData *rd, int cmd,
        void *data_out, size_t data_out_len) {

    // Send ANSWER to L0 user
    if(rd->l0_f)
        ksnLNullSendToL0(ke,
                rd->addr, rd->port, rd->from, rd->from_len,
                cmd,
                data_out, data_out_len);

    // Send ANSWER to peer
    else
        ksnCoreSendto(ke->kc, rd->addr, rd->port,
                cmd,
                data_out, data_out_len);
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
            ksnTDBnamespaceSet(ke->kf, DEFAULT_NAMESPACE);
            break;

        case EV_K_STOPPED_BEFORE:
            break;

        // DATA received
        case EV_K_RECEIVED:
        {
            // DATA event
            ksnCorePacketData *rd = data;

            switch(rd->cmd) {

                // Set data request #129
                case CMD_D_SET:
                {
                    // Get type of request JSON - 1 or BINARY - 0
                    const int data_type = get_data_type();
                    void *updated_key = NULL;
                    size_t updated_key_length = 0;

                    // JSON data
                    if(data_type) {

                        // Get JSON string from input data
                        char *json_data_unesc = data_to_json_str(rd->data, rd->data_len);
                        ksn_printf(ke, APPNAME, DEBUG,
                                "CMD_D_SET data: %s\n", json_data_unesc);

                        // Parse request
                        json_param jp = { NULL, NULL, NULL, NULL, NULL };
                        json_parse(json_data_unesc, &jp);

                        // Free JSON string
                        free(json_data_unesc);

                        // Process the data
                        int rv = -1;
                        if(jp.key != NULL && jp.key[0]) {

                            // Set/update data
                            if(jp.data != NULL && jp.data[0])
                                rv = ksnTDBsetStr(ke->kf, jp.key, jp.data,
                                    strlen(jp.data) + 1);
                            // Remove key
                            else
                                rv = ksnTDBdeleteStr(ke->kf, jp.key);
                        }
                        if(!rv) {
                            ksn_printf(ke, APPNAME, DEBUG,
                                "KEY: \"%s\", DATA: \"%s\", DB set status: %d\n",
                                jp.key, jp.data, rv);

                            updated_key_length = strlen(jp.key) + 1;
                            updated_key = memdup(jp.key, updated_key_length);

                            //updated_key = ksnet_formatMessage("{ \"key\": \"%s\" }", jp.key);
                            //updated_key_length = strlen(updated_key) + 1;
                        }
                        else
                            ksn_printf(ke, APPNAME, DEBUG,
                                "An error happened during write to DB, "
                                "KEY: \"%s\", DATA: \"%s\", "
                                "rv: %d ...\n", jp.key, jp.data, rv);

                        // Free parameters data
                        json_free(&jp);
                    }

                    // BINARY data
                    else {

                        // Parse Binary data structure
                        teo_db_data *tdd = rd->data;

                        // Save data to DB
                        int rv = -1;
                        if(tdd->key_length &&
                           tdd->key_length + tdd->data_length +
                                sizeof(teo_db_data) == rd->data_len) {

                            // Add or update key
                            if(tdd->data_length)
                                rv = ksnTDBset(ke->kf,
                                    tdd->key_data,
                                    tdd->key_length,
                                    tdd->key_data + tdd->key_length,
                                    tdd->data_length
                                );
                            // Remove key
                            else
                                rv = ksnTDBdelete(ke->kf, tdd->key_data,
                                    tdd->key_length
                                );

                            if(!rv) {
                                ksn_printf(ke, APPNAME, DEBUG,
                                    "KEY: %s, DATA: %s, DB set status: %d\n",
                                    tdd->key_data,
                                    tdd->key_data + tdd->key_length, rv);

                                updated_key_length = tdd->key_length;
                                updated_key = memdup(tdd->key_data, updated_key_length);
                            }
                        }
                        else
                            ksn_printf(ke, APPNAME, DEBUG,
                                    "Wrong request DB packet size %d...\n",
                                    rd->data_len);

                        if(rv) { // \todo check error if != 0
                            ksn_printf(ke, APPNAME, DEBUG,
                                    "An error happened during write to DB, "
                                    "rv: %d ...\n", rv);
                        }
                    }

                    // Send event to subscribers
                    if(updated_key != NULL) {
                        if(updated_key_length) {
                            teoSScrSend(ke->kc->kco->ksscr, EV_D_SET,
                                    updated_key, updated_key_length, 0);
                        }
                        free(updated_key);
                    }
                }
                break;

                // Get data request #130
                case CMD_D_GET:
                {
                    // Get type of request JSON - 1 or BINARY - 0
                    const int data_type = get_data_type();

                    // JSON data
                    if(data_type) {

                        // Get JSON string from input data
                        char *json_data_unesc = data_to_json_str(rd->data,
                                rd->data_len);
                        ksn_printf(ke, APPNAME, DEBUG,
                                "CMD_D_GET data: %s\n", json_data_unesc);

                        // Parse request
                        json_param jp = { NULL, NULL, NULL, NULL, NULL };
                        json_parse(json_data_unesc, &jp);

                        // Free JSON string
                        free(json_data_unesc);

                        // Process the data
                        size_t data_len;
                        char *data = ksnTDBgetStr(ke->kf, jp.key, &data_len);
                        ksn_printf(ke, APPNAME, DEBUG,
                                "KEY: %s, DATA: %s, DB get status: %d\n",
                                jp.key, data != NULL ? data : "", data == NULL);

                        // Prepare ANSWER data
                        void *data_out = ksnet_formatMessage(
                            "{ \"key\":\"%s\", \"data\":\"%s\", \"id\":\"%s\" }",
                            jp.key,
                            data != NULL ? data : "",
                            jp.id != NULL ? jp.id : ""
                        );
                        size_t data_out_len = strlen(data_out) + 1;

                        // Free DB data
                        free(data);

                        // Send request answer
                        send_answer(ke, rd, CMD_D_GET_ANSWER, data_out,
                                data_out_len);                        

                        // Free out data
                        free(data_out);

                        // Free parameters data
                        json_free(&jp);
                    }

                    // BINARY data
                    else {

                        // Parse Binary data structure
                        teo_db_data *tdd = rd->data;

                        if(tdd->key_length &&
                           tdd->key_length + tdd->data_length +
                                sizeof(teo_db_data) == rd->data_len) {

                            // Get data from DB
                            size_t data_len;
                            char *data = ksnTDBget(ke->kf, tdd->key_data,
                                    tdd->key_length, &data_len);

                            if(data != NULL) {

                                ksn_printf(ke, APPNAME, DEBUG,
                                    "Get value from DB: KEY: %s, DATA: %s\n",
                                    tdd->key_data, data);
                            }
                            else  {

                                ksn_printf(ke, APPNAME, DEBUG,
                                    "The KEY %s not found in DB\n",
                                    tdd->key_data);
                            }

                            // Create output data
                            size_t data_out_len;
                            teo_db_data *data_out = prepare_request_data(
                                tdd->key_data, tdd->key_length,
                                data != NULL ? data : null_str , data_len,
                                tdd->id, &data_out_len
                            );

                            // Send request answer
                            send_answer(ke, rd, CMD_D_GET_ANSWER,
                                    data_out, data_out_len);

                            // Free out data
                            free(data_out);
                        }
                        else
                            ksn_printf(ke, APPNAME, DEBUG,
                                    "Wrong request DB packet size %d...\n",
                                    rd->data_len);
                    }
                }
                break;

                // List keys request #131 or List keys length request #134
                case CMD_D_LIST:
                case CMD_D_LIST_RANGE:
                case CMD_D_LIST_LENGTH:
                {
                    // Get type of request JSON - 1 or BINARY - 0
                    const int data_type = get_data_type();
                    char *key_str = NULL;
                    char *id_str = NULL;
                    char *from_str = NULL;
                    char *to_str = NULL;

                    // JSON data
                    if(data_type) {

                        // Get JSON string from input data
                        char *json_data_unesc = data_to_json_str(rd->data,
                                rd->data_len);
                        ksn_printf(ke, APPNAME, DEBUG,
                                    "CMD_D_LIST data: %s\n", json_data_unesc);

                        // Parse request
                        json_param jp = { NULL, NULL, NULL, NULL, NULL };
                        json_parse(json_data_unesc, &jp);

                        key_str = jp.key;
                        id_str = jp.id;
                        from_str = jp.from;
                        to_str = jp.to;

                        // Free JSON string
                        free(json_data_unesc);
                        if(jp.data != NULL) free(jp.data);
                    }

                    // Binary
                    else {
                        teo_db_data *tdd = rd->data;
                        key_str = tdd->key_length ? strdup(tdd->key_data) : NULL;
                    }

                    // Process the data
                    uint8_t cmd_answer = CMD_D_ERROR_ANSWER;
                    size_t out_data_len = 0;
                    void *out_data = NULL;
                    ksnet_stringArr argv;
                    
                    uint32_t list_len = ksnTDBkeyList(ke->kf, key_str, &argv);
                    ksn_printf(ke, APPNAME, DEBUG,
                            "LIST_LEN: %d\n",
                            list_len);

                    // List keys length request #134
                    if(rd->cmd == CMD_D_LIST_LENGTH) {
                        
                        cmd_answer = CMD_D_LIST_LENGTH_ANSWER;
                        
                        // JSON data
                        if(data_type) {
                            
                            out_data = ksnet_formatMessage(
                                "{ \"key\":\"%s\", \"id\": \"%s\", \"listLen\": %d }", 
                                key_str,
                                id_str != NULL ? id_str : "",
                                list_len
                            );
                            out_data_len = strlen(out_data) + 1;
                        }
                        
                        // Binary
                        else {
                            
                            // Parse Binary data structure
                            teo_db_data *tdd = rd->data;
                            
                            // Create output data
                            out_data = prepare_request_data(
                                key_str, tdd->key_length,
                                &list_len, sizeof(list_len),
                                tdd->id, &out_data_len
                            );
                        }                        
                    }
                    
                    // List keys request #131
                    else if(rd->cmd == CMD_D_LIST || rd->cmd == CMD_D_LIST_RANGE) {
                        
                        cmd_answer = CMD_D_LIST_ANSWER;

                        // JSON data
                        if(data_type) {
                            
                            // Select range
                            int from = 0, to = list_len;                            
                            if(rd->cmd == CMD_D_LIST_RANGE) {
                                from = from_str != NULL ? atoi(from_str) : 0;
                                to = to_str != NULL ? atoi(to_str) : 0;
                                if(from > list_len) from = list_len;
                                if(to > list_len) to = list_len;
                                cmd_answer = CMD_D_LIST_RANGE_ANSWER;
                            }

                            // Prepare ANSWER data
                            char *ar_str = ksnet_formatMessage("[ ");
                            int i; for(i = from; i < to; i++) {
                                ar_str = ksnet_sformatMessage(ar_str,
                                    "%s%s{ \"key\":\"%s\", \"id\": \"%s\" }",
                                    ar_str, i>from ? ", " : "", argv[i],
                                    id_str != NULL ? id_str : "");
                            }
                            ar_str = ksnet_sformatMessage(ar_str, "%s ]", ar_str);

                            out_data = ar_str;
                            out_data_len = strlen(ar_str) + 1;
                        }

                        // Binary
                        else {

                            int from = 0, to = list_len;
                            
                            // Parse Binary data structure
                            teo_db_data *tdd = rd->data;
                            size_t ptr;
                            
                            // CMD_D_LIST_RANGE extended data: { UIND32_t, UIND32_t }
                            if(rd->cmd == CMD_D_LIST_RANGE) {                                
                                ptr = tdd->key_length;
                                from = *(uint32_t*)(tdd->key_data + ptr); ptr += sizeof(uint32_t);
                                to = *(uint32_t*)(tdd->key_data + ptr); ptr += sizeof(uint32_t);
                                if(from > list_len) from = list_len;
                                if(to > list_len) to = list_len;
                                cmd_answer = CMD_D_LIST_RANGE_ANSWER;
                            }

                            // Prepare ANSWER data
                            size_t ar_bin_len = sizeof(uint32_t); 
                            void *ar_bin = malloc(ar_bin_len);
                            // Number of keys
                            *(uint32_t *)ar_bin = to - from; ptr = ar_bin_len;
                            // List of keys with 0 at and
                            int i; for(i = from; i < to; i++) {
                                size_t len = strlen(argv[i]) + 1;
                                ar_bin_len += len;
                                ar_bin = realloc(ar_bin, ar_bin_len);
                                memcpy(ar_bin + ptr, argv[i], len);
                                ptr = ar_bin_len;
                            }

                            // Create output data
                            out_data = prepare_request_data(
                                tdd->key_data, tdd->key_length,
                                ar_bin,ar_bin_len,
                                tdd->id, &out_data_len
                            );

                            // Free binary array
                            free(ar_bin);
                        }
                    }
                    
                    // Send request answer
                    if(cmd_answer) {
                        send_answer(ke, rd, cmd_answer, out_data, out_data_len);
                    }

                    // Free used data
                    if(out_data != NULL) free(out_data);
                    if(id_str != NULL) free(id_str);
                    if(key_str != NULL) free(key_str);
                    if(from_str != NULL) free(from_str);
                    if(to_str != NULL) free(to_str);
                    ksnet_stringArrFree(&argv);
                }
            }
        }
        break;

        default: break;
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
    teoSetAppVersion(ke, TDB_VERSION);

    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
