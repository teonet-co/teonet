#!/bin/sh

# Upgrade Ubuntu
sudo apt-get update
sudo apt-get upgrade

# Autoconf dependence
sudo apt-get -y install autoconf intltool libtool libglib2.0-dev doxygen make gcc

# Project dependence
sudo apt-get install -y libssl-dev libev-dev libconfuse-dev uuid-dev

# Libtuntap dependence
sudo apt-get install -y cmake g++ 

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

