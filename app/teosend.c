/** 
 * File:   teosend.c
 * Author: Kirill Scherba
 *
 * Created on July 15, 2015, 5:48 PM
 * 
 * Test application to send and receive teonet messages.
 * 
 * - subscribe to timer event
 * - send message by timer
 * - show received messages
 * - show idle event
 * 
 */

#include <stdio.h>
#include <stdlib.h>

#include "ev_mgr.h"

#define TVPN_VERSION VERSION

/*
 * 
 */
int main(int argc, char** argv) {
    
    printf("Teosend ver " TVPN_VERSION "\n");

    return (EXIT_SUCCESS);
}
