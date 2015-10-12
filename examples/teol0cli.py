#!/usr/bin/env python
 
import socket
import _teonet_l0_client


host_name = "C2"
TCP_IP = "127.0.0.1"
TCP_PORT = 9000
msg = "Hello"
peer_name = "teostream"
BUFFER_SIZE = 2048
packet = "111111111111111111111111111111111111111111111111111111111111111111111111111111"
CMD_ECHO = 65
 
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

#pkg = _teonet_l0_client.teoLNullInit(packet, KSN_BUFFER_SIZE)
pkg = _teonet_l0_client.teoLNullPacketCreate(packet, BUFFER_SIZE, 0, "", host_name, 3)
s.send(pkg)
pkg = _teonet_l0_client.teoLNullPacketCreate(packet, BUFFER_SIZE, CMD_ECHO, peer_name, msg, 6)
s.send(pkg)