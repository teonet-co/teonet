#!/bin/sh

# Update Centos
yum update

# Autoconf dependence
yum install -y autoconf intltool libtool glib2-devel doxygen make gcc

# Project dependence
yum install -y openssl-devel libev-devel libuuid-devel

# Install confuse
cd distr
#yum install -y wget
#wget http://savannah.nongnu.org/download/confuse/confuse-2.7.tar.gz
tar -xzvf confuse-2.7.tar.gz
cd confuse-2.7
./configure
cd src
cp Makefile Makefile.old
patch < ../../confuse-2.7-src-Makefile.patch 
make
make install
cd ../..

# Libtuntap dependence
yum install -y cmake gcc-c++ unzip

# Get libtuntap
cd distr
unzip libtuntap.zip
cd libtuntap-master

# Libtuntap build
cmake ./
make
make install
cd ../..
