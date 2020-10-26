#!/bin/sh

# Set project dependence to make Teonet project from sources at empty host

# Upgrade Ubuntu
sudo apt-get update
sudo apt-get -y upgrade

# Autoconf dependence
sudo apt-get -y install autoconf intltool libtool libglib2.0-dev doxygen make gcc 

# Project dependence
sudo apt-get install -y libssl-dev libev-dev libconfuse-dev uuid-dev libcunit1-dev cpputest libcurl4-openssl-dev

# Libtuntap dependence
sudo apt-get install -y cmake g++ unzip

# Get libtuntap
cd distr
unzip libtuntap.zip
cd libtuntap-master

# Libtuntap build
cmake ./
make
sudo make install

# Remove libtuntap source
cd ..
rm -fr libtuntap-master

# Update system dynamic libraries configuration
sudo ldconfig
