/** 
 * File:   make_package.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Build package
 *
 * Created on August 24, 2015, 2:22 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/config/conf.h"
#include "../src/config/config.h"

#define TBP_VERSION "0.0.1"

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teonet build package ver. %s, %s\n", TBP_VERSION, COPYRIGHT);
    
    char *CI_BUILD_REF = getenv("CI_BUILD_REF");
    printf("CI_BUILD_REF=%s\n", CI_BUILD_REF);
    
    int rv = EXIT_FAILURE;
    
    // Check for required arguments
    if(argc < 2) {
        
        printf( "\n"
                "Usage: %s LINUX ARCH\n"
                "\n"
                "Where LINUX: deb - DEBIAN\n"
                "      ARCH: architecture (default: amd64)\n"
                "\n"
                , argv[0]);
        return (EXIT_FAILURE);
    }
    
    // Make DEBIAN repository
    if(!strcmp(argv[1], "deb")) {
        
        // Import repository keys
        if(system("sh/make_deb_keys_add.sh")) return (EXIT_FAILURE);
        
        // Create version
        char version[KSN_BUFFER_SM_SIZE];
        char *CI_BUILD_ID = getenv("CI_BUILD_ID");
        
        if(CI_BUILD_ID != NULL) {
            snprintf(version, KSN_BUFFER_SM_SIZE, "%s-%s", VERSION, CI_BUILD_ID);
            // TODO: Set version to configure.ac 
        }
        else 
            snprintf(version, KSN_BUFFER_SM_SIZE, "%s", VERSION);

        // Execute build repository script
        char cmd[KSN_BUFFER_SM_SIZE];
        snprintf(cmd, KSN_BUFFER_SM_SIZE, "sh/make_%s.sh %s %s", 
                argv[1], version, argc >= 3 ? argv[2] : "");
        
        rv = system(cmd);
    }
    
    return rv;
}
