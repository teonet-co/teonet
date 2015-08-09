#!/bin/sh

# Update Centos
zypper -y update

# Autoconf dependence
zypper install -y autoconf intltool libtool glib2-devel doxygen make gcc patch

# Project dependence
zypper install -y openssl-devel libev-devel libuuid-devel

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
cd ..
make
make install
cd ../..

# Libtuntap dependence
zypper install -y cmake gcc-c++ unzip

# Get libtuntap
cd distr
unzip libtuntap.zip
cd libtuntap-master

# Libtuntap build
cmake ./
make
make install
cd ../..

# Update system dynamic libraries configuration
ldconfig
