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

#include "ev_mgr.h"

#define TDB_VERSION "0.0.1"

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teodb ver " TDB_VERSION ", based on teonet ver. "
            VERSION "\n");

    return (EXIT_SUCCESS);
}
