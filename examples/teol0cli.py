#!/usr/bin/env python

#
# Simple Teonet python L0 client sample.
#
# In this example we connect to L0 server, send initialize message, send a 
# CMD_ECHO command and receive sent message.
#
#
# Execute next commands to build teonet_l0_client module:
#
# cd /teonet/embedded/teocli/python
# ./make_python.sh
#
 
import socket
import _teocli

# Teonet L0 server parameters
#
host_name = "C2"
TCP_IP = "gt1.kekalan.net"
TCP_PORT = 9010

# Packet buffer
#
BUFFER_SIZE = 2048
packet = "\0"*(BUFFER_SIZE)

# Command parameters
#
CMD_ECHO = 65
peer_name = "ps-server"
message = "Hello world from python Teonet L0 client sample!"

# Welcome message
#
print "Teonet python L0 client example, ver. 0.0.1\n"
 
# Create socket and connect to L0 server
#
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

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

# Read message from peer
# The "Peer name" server will send message to this client with message we send in previous command
#
packet = s.recv(BUFFER_SIZE)
for i in range(8, BUFFER_SIZE-1):
  if packet[i] == '\0': break
from_peer = packet[8:i]
data = packet[i+1:]
print "Receive 24 bytes:", len(data), "bytes data from L0 server, from peer", from_peer, ", data:", data

# Close connection
#
s.close()
print "done"
