#!/bin/sh

# Set project dependence to make Teonet project from sources at empty host

# Update OpenSuse
zypper -y update

# Autoconf dependence
zypper install -y autoconf intltool libtool glib2-devel doxygen make gcc patch

# Project dependence
zypper install -y openssl-devel libev-devel libuuid-devel

# Install Suse cUnit project dependence
zypper install cunit-devel
# for opensuse less than 13.2:
#zypper addrepo -fg http://download.opensuse.org/repositories/home:Strahlex/openSUSE_13.2/home:Strahlex.repo
#zypper refresh 
#zypper install cunit-devel

# Install confuse
cd distr
#zypper install -y wget
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

# Remove libtuntap source
cd ..
rm -fr libtuntap-master
cd ..

# Update system dynamic libraries configuration
ldconfig
