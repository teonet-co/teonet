/** 
 * File:   make_package.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Crete package
 *
 * Created on August 24, 2015, 2:22 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../src/config/conf.h"
#include "../src/config/config.h"

/**
 * Main application function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv) {
    
    if(argc < 2) {
        
        printf("Usage: %s LINUX\n"
               "where LINUX is: deb - DEBIAN\n", argv[0]);
        return (EXIT_FAILURE);
    }
    
    char cmd[KSN_BUFFER_SM_SIZE];
    snprintf(cmd, KSN_BUFFER_SM_SIZE, "sh/make_%s.sh %s", argv[1], VERSION);
    
    return system(cmd);        
}
