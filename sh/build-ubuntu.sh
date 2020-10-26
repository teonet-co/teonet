#!/bin/sh

# Set project dependence to make Teonet project from sources at empty host

# Upgrade Ubuntu
apt-get update
apt-get -y upgrade

# Autoconf dependence
apt-get -y install autoconf intltool libtool libglib2.0-dev doxygen make gcc 

# Project dependence
apt-get install -y libssl-dev libev-dev libconfuse-dev uuid-dev libcunit1-dev cpputest libcurl4-openssl-dev

# Libtuntap dependence
apt-get install -y cmake g++ unzip

# Get libtuntap
cd distr
unzip libtuntap.zip
cd libtuntap-master

# Libtuntap build
cmake ./
make
make install

# Remove libtuntap source
cd ..
rm -fr libtuntap-master

# Update system dynamic libraries configuration
ldconfig
