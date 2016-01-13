/** 
 * \file   teoterm.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teoterm.c
 * 
 * ## Teonet terminal custom command
 * 
 * This examples shows how to add custom command to the Teonet terminal
 *
 * Created on July 27, 2015, 3:56 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "ev_mgr.h"
#include "modules/net_cli.h"


#define TTERM_VERSION VERSION

/**
 * Show user command
 *
 * @param cli The handle of the cli structure
 * @param command The entire command which was entered. This is after command expansion
 * @param argv The list of arguments entered
 * @param argc The number of arguments entered
 * @return
 */
int cmd_user(struct cli_def *cli, const char *command, char *argv[], int argc) {

    cli_print(cli, "User command executed\r\n");

    return CLI_OK;
}


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
        case EV_K_TERM_STARTED:
        {
            // Tunnel parameters message
            printf("The terminal server command 'user' was added\n");
            
            cli_register_command(ke->cli, NULL, "user", cmd_user, 
                    PRIVILEGE_UNPRIVILEGED, MODE_EXEC, 
                    "Show list of teonet peers");
          
        }
        break;
        
        default:
            break;
    }
}
        
/**
 * Main application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 * 
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

    printf("Teoterm example ver " TTERM_VERSION "\n");

    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION);
    
    // Start teonet
    ksnetEvMgrRun(ke);
    
    return 0;
}
