/** 
 * File:   teonet_lo_client.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on October 12, 2015, 12:32 PM
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "teonet_l0_client.h"

/**
 * Create L0 client packet
 *  
 * @param buffer Buffer to create packet in
 * @param buffer_length Buffer length
 * @param command Command to peer
 * @param peer Teonet peer
 * @param data Command data
 * @param data_length Command data length
 * 
 * @return Length of packet
 */
size_t teoLNullPacketCreate(char* buffer, size_t buffer_length, 
        uint8_t command, const char * peer, const void* data, size_t data_length) {
    
    // \todo Check buffer length
    
    teoLNullCPacket* pkg = (teoLNullCPacket*) buffer;
    
    pkg->cmd = command;
    pkg->data_length = data_length;
    pkg->peer_name_length = strlen(peer) + 1;
    memcpy(pkg->peer_name, peer, pkg->peer_name_length);
    memcpy(pkg->peer_name + pkg->peer_name_length, data, pkg->data_length);
    pkg->checksum = teoByteChecksum(pkg->peer_name, pkg->peer_name_length + 
            pkg->data_length);
    pkg->header_checksum = teoByteChecksum(pkg, sizeof(teoLNullCPacket) - 
            sizeof(pkg->header_checksum));
    
    return sizeof(teoLNullCPacket) + pkg->peer_name_length + pkg->data_length;
}

/**
 * Create initialize L0 client packet
 * 
 * @param buffer Buffer to create packet in
 * @param buffer_length Buffer length
 * @param host_name Name of this L0 client
 * 
 * @return Pointer to teoLNullCPacket
 */
size_t teoLNullInit(char* buffer, size_t buffer_length, const char* host_name) {
    
    return teoLNullPacketCreate(buffer, buffer_length, 0, "", host_name, 
            strlen(host_name) + 1);
}

/**
 * Calculate checksum
 * 
 * Calculate byte checksum in data buffer
 * 
 * @param data Pointer to data buffer
 * @param data_length Length of the data buffer to calculate checksum
 * 
 * @return Byte checksum of the input buffer
 */
uint8_t teoByteChecksum(void *data, size_t data_length) {
    
    int i;
    uint8_t *ch, checksum = 0;
    for(i = 0; i < data_length; i++) {
        ch = (uint8_t*)(data + i);
        checksum += *ch;
    }
    
    return checksum;
}

/**
 * Set socket or FD to non blocking mode
 * 
 * @param fd
 */
void set_nonblock(int fd) {

    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * Set TCP NODELAY option
 * 
 * @param fd TCP socket descriptor
 * 
 * @return Result of setting. Success if >= 0.
 */
int set_tcp_nodelay(int fd) {

    int flag = 1;
    int result = setsockopt(fd,           // socket affected
                         IPPROTO_TCP,     // set option at TCP level
                         TCP_NODELAY,     // name of option
                         (char *) &flag,  // the cast is historical cruft
                         sizeof(int));    // length of option value
    if (result < 0) {
        
        printf("Set TCP_NODELAY of fd %d error\n", fd);
    }
    
    return result;
}

/**
 * Create TCP client and connect to server
 * 
 * @param server Server IP or name
 * @param port Server port
 * 
 * @return Socket description: > 0 - success connection
 */
int teoLNullClientCreate(int port, const char *server) {

    /* Variable and structure definitions. */
    int sd, rc;
    struct sockaddr_in serveraddr;
    struct hostent *hostp;

    /* The socket() function returns a socket */
    /* descriptor representing an endpoint. */
    /* The statement also identifies that the */
    /* INET (Internet Protocol) address family */
    /* with the TCP transport (SOCK_STREAM) */
    /* will be used for this socket. */
    /******************************************/
    /* get a socket descriptor */
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    
        printf("Client-socket() error\n");
        return(-1);
    }
    else {
        printf("Client-socket() OK\n");
    }

    printf("Connecting to the f***ing %s at port %d ...\n", server, port);

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    if((serveraddr.sin_addr.s_addr = inet_addr(server)) == (unsigned long) INADDR_NONE) {  

        /* When passing the host name of the server as a */
        /* parameter to this program, use the gethostbyname() */
        /* function to retrieve the address of the host server. */
        /***************************************************/
        /* get host address */
        hostp = gethostbyname(server);
        if(hostp == (struct hostent *)NULL) {
        
            printf("HOST NOT FOUND --> h_errno = %d\n", h_errno);
            close(sd);
            return(-2);
        }
        memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
    }

    /* After the socket descriptor is received, the */
    /* connect() function is used to establish a */
    /* connection to the server. */
    /***********************************************/
    /* connect() to server. */
    if((rc = connect(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0) {
    
        printf("Client-connect() error\n");
        close(sd);
        return(-3);
    }
    else {
        printf("Connection established...\n");
    }

    // Set non block mode
    set_nonblock(sd);
    
    // Set TCP_NODELAY option
    set_tcp_nodelay(sd);

    return sd;
}
