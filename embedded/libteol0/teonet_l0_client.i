/* teonet_l0_client.i */
%module teonet_l0_client
%{
/* Put header files here or function declarations like below */
#include "teonet_l0_client.h"    
%}

extern uint8_t teoByteChecksum(void *data, size_t data_length);
extern void * teoLNullPacketCreate(char* buffer, size_t buffer_length, 
        int command, char * peer, char * data, size_t data_length);