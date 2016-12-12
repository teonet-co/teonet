# The MIT License
#
# Copyright 2016 Kirill Scherba <kirill@scherba.ru>.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Teonet python L0 async client example

import socket
import select
import _teocli
import sys 


# Event callback
#
def event_cb(con, event, packet, packet_len, user_data):
    
    event_cb.num += 1
    
    if event == EV_L_CONNECTED:
        
        # Show message
        print event_cb.num, ": Connected"
        
    elif event == EV_L_DISCONNECTED:
        
        # Show message
        print event_cb.num, ": Disconnected"
        
    elif event == EV_L_RECEIVED:
        
        # Parse packet
        #
        for i in range(8, BUFFER_SIZE-1): 
            if packet[i] == '\0': break
        from_peer = packet[8:i]
        data = packet[i+1:]
        cmd = ord(packet[0])
        
        # Show message
        #
        print event_cb.num, ": Receive", packet_len, "bytes:", len(data)-1, "bytes data from L0 server, from peer", from_peer, ", cmd:", cmd, ", data:", data


# Static variable initialization
#
event_cb.num = 0


# Commands constant
#
CMD_ECHO = 65
CMD_ECHO_ANSWER = 66


# Eventss constant
#
EV_L_CONNECTED = 0      #< After connected to L0 server
EV_L_DISCONNECTED = 1   #< After disconnected from L0 server
EV_L_RECEIVED = 2       #< Data received


# Main
#
if __name__ == "__main__":
    
    # Teonet L0 server parameters
    #
    host_name = "py-Client"
    TCP_IP = "gt1.kekalan.net"
    TCP_PORT = 9010

    # Packet buffer
    #
    BUFFER_SIZE = 2048
    packet = "\0"*(BUFFER_SIZE)

    # Parameters
    #
    peer_name = "ps-server"
    message = "Hello world from python Teonet L0 async client sample!"

    # Welcome message
    #
    print "Teonet python L0 async client example, ver. 0.0.1\n"
 
    # Create socket and connect to L0 server
    #
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TCP_IP, TCP_PORT))
    event_cb(s, EV_L_CONNECTED, "", 1, 0)

    # Send initialize (need after connection):
    # Send a packet with cmd = 0, peer_name = "", message = This host name, massage_length = message length + 1
    #
    print "Send", 8 + 1 + len(host_name) + 1, "bytes initialize packet to L0 server"
    packet_len = _teocli.teoLNullPacketCreateLogin(packet, BUFFER_SIZE, host_name)
    for i in range(0, packet_len):
      s.send(packet[i])
      
    # Send CMD_ECHO command
    # Send a packet with cmd = CMD_ECHO, peer_name = Peer name, message = Any message, massage_length = message length + 1
    #
    print "Send", 8 + len(peer_name) + 1 + len(message) + 1, "bytes packet to L0 server to peer", peer_name, ", data:", message
    packet_len = _teocli.teoLNullPacketCreate(packet, BUFFER_SIZE, CMD_ECHO, peer_name, message, len(message) + 1)
    for i in range(0, packet_len):
      s.send(packet[i])
      
    # Read event loop
    #
    input = [s,sys.stdin]
    running = 1 
    #num = 0
    while running: 
        inputready,outputready,exceptready = select.select(input,[],[]) 

        for ser in inputready:  
            
            if ser == s: 
                
                # Read message from peer
                # The "Peer name" server will send message to this client with message we send in previous command
                #
                packet = s.recv(BUFFER_SIZE)
                
                # Parse packet
                #
                for i in range(8, BUFFER_SIZE-1):
                  if packet[i] == '\0': break
                from_peer = packet[8:i]
                data = packet[i+1:]
                cmd = ord(packet[0])
                
                # Answer to echo command
                #
                if cmd == 65:
                    packet_out = "\0"*(BUFFER_SIZE)
                    packet_len = _teocli.teoLNullPacketCreate(packet_out, BUFFER_SIZE, CMD_ECHO_ANSWER, from_peer, data, len(data) + 1)
                    for i in range(0, packet_len):
                        s.send(packet_out[i])
                        
                # Call event callback
                event_cb(s, EV_L_RECEIVED, packet, len(packet), 0)
            
    # Close connection
    #
    s.close()
    event_cb(s, EV_L_DISCONNECTED, "", 1, 0)
