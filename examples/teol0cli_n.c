/**
 *  
 * \file   teol0cli_n.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teol0cli_n.c
 * 
 * Native L0 client example
 *
 * Created on October 13, 2015, 6:12 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "../embedded/libteol0/teonet_l0_client.h"

#define TL0CN_VERSION "0.0.1"  

/**
 * Main L0 Native client example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 * 
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teol0cli_n example ver " TL0CN_VERSION " (native client)\n");
    
    // Packet buffer
    #define BUFFER_SIZE 2048
    char packet[BUFFER_SIZE];
    teoLNullCPacket *pkg = (teoLNullCPacket*) packet;
    
    // \todo Connect to L0 server
    
    // Initialize L0 connection
    size_t snd;
    char *host_name = "C3";
    size_t pkg_length = teoLNullInit(packet, BUFFER_SIZE, host_name);
    // \todo    if((snd = write(fd, pkg, pkg_length)) >= 0);                
    printf("Send %d bytes initialize packet to L0 server\n", (int)snd);
    
    // \todo Send command message
    
    // \todo Receive answer from server
    
    // \todo Close connection
           
    return (EXIT_SUCCESS);
}
