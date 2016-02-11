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
 * Check data type and show message
 */
#define get_data_type() rd->data_len && \
                        rd->data_len > sizeof(JSON) && \
                        !strcmp(rd->data, JSON)  ? 1 : 0; \
    \
    ksnet_printf(&ke->ksn_cfg, DEBUG, APPNAME \
        "Got cmd %d, type %s, from %s\n", \
        rd->cmd, data_type  ? JSON : BINARY, rd->from \
    )

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

        // DATA received
        case EV_K_RECEIVED:
        {
            // DATA event
            ksnCorePacketData *rd = data;

            // Request type
            const char *JSON = "JSON";
            const char *BINARY = "BINARY";

            switch(rd->cmd) {

                // Set data request
                case CMD_D_SET:
                {
                    // Get type of request JSON - 1 or BINARY - 0
                    const int data_type = get_data_type();

                }
                break;

                // Get data request
                case CMD_D_GET:
                {
                    // Get type of request JSON - 1 or BINARY - 0
                    const int data_type = get_data_type();
                    
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
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb /*NULL*/, READ_ALL);

    // Set application type
    teoSetAppType(ke, "teo-db");

    // Start teonet
    ksnetEvMgrRun(ke);

    return (EXIT_SUCCESS);
}
