#!/bin/sh

#sudo kill 18108
sudo app/teovpn teo-gbs -a 5.63.158.100 -r 9010 -p 9010 --network teonet --vpn_start --vpn_ip 172.27.101.54 --show_peer --l0_allow --l0_tcp_port 9010 -k
sudo app/teovpn teo-gbs -a 5.63.158.100 -r 9010 -p 9010 --network teonet --vpn_start --vpn_ip 172.27.101.54 --show_peer --l0_allow --l0_tcp_port 9010 -d
