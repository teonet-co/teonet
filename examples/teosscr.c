/* 
 * \file   teosscr.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * \example   teosscr.c
 * 
 * ## Using the Teonet Subscribe module
 * Module: subscribe.c  
 *   
 * This is example application to subscribe one teonet application to other 
 * teonet application event. 
 * 
 * In this example we:
 * * start one application as server: ```examples/teosscr teosscr-ser```
 * * start other application as client: ```examples/teosscr teosscr-cli```
 * 
 * Created on January 5, 2016, 3:38 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TSSCR_VERSION "0.0.1"

/**
 * Main QUEUE callback example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 * 
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teosscr example ver " TSSCR_VERSION ", "
           "based on teonet ver. " VERSION "\n");

    return (EXIT_SUCCESS);
}
