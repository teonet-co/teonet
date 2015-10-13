/**
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
#include <string.h>
#include <unistd.h>

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
    
    // Teonet L0 server parameters
    const char *peer_name = "teostream";
    const char *host_name = "C3";
    const char *TCP_IP = "127.0.0.1";
    const int TCP_PORT = 9000;
    #define CMD_ECHO 65
    
    // Packet buffer
    #define BUFFER_SIZE 2048
    char packet[BUFFER_SIZE];
    teoLNullCPacket *pkg = (teoLNullCPacket*) packet;
    
    // Connect to L0 server
    int fd = teoLNullClientCreate(TCP_PORT, TCP_IP);
    if(fd > 0) {
        
        // Initialize L0 connection
        size_t snd;
        size_t pkg_length = teoLNullInit(packet, BUFFER_SIZE, host_name);
        if((snd = write(fd, pkg, pkg_length)) >= 0);                
        printf("Send %d bytes initialize packet to L0 server\n", (int)snd);

        // Send command message
        const char *msg = "Hello";
        pkg_length = teoLNullPacketCreate(packet, BUFFER_SIZE, CMD_ECHO, 
                peer_name, msg, strlen(msg) + 1);
        if((snd = write(fd, pkg, pkg_length)) >= 0);
        printf("Send %d bytes packet to L0 server to peer %s, data: %s\n", 
               (int)snd, peer_name, msg);

        // \todo If uncomment return below the peer_name peer break with Segmentation fault
        // return (EXIT_SUCCESS);
        
        // Receive answer from server
        char buf[BUFFER_SIZE];
        size_t rc;
        while((rc = read(fd, buf, BUFFER_SIZE)) == -1);

        // Process received data
        if(rc > 0) {

            teoLNullCPacket *cp = (teoLNullCPacket*)buf;
            char *data = cp->peer_name + cp->peer_name_length;

            printf("Receive %d bytes: %d bytes data from L0 server, "
                    "from peer %s, data: %s\n", 
                    (int)rc, cp->data_length, cp->peer_name, data);
        }

        // Close connection
        close(fd);
    }
    
    return (EXIT_SUCCESS);
}
