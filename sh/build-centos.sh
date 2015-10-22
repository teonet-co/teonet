#!/bin/sh

# Set project dependence to make Teonet project from sources at empty host

# Update Centos
yum -y update

# Autoconf dependence
yum install -y autoconf intltool libtool glib2-devel doxygen make gcc

# Project dependence
yum install -y openssl-devel libev-devel libuuid-devel

# Install CentOS cUnit project dependence
yum install -y wget bzip2
wget http://sourceforge.net/projects/cunit/files/CUnit/2.1-3/CUnit-2.1-3.tar.bz2
tar -xvf CUnit-2.1-3.tar.bz2
cd CUnit-2.1-3

libtoolize --force
aclocal
autoheader
automake --force-missing --add-missing
autoconf
chmod u+x configure
./configure --prefix=/usr
make
make install
cd ..
rm -fr CUnit-2.1-3
rm -f CUnit-2.1-3.tar.bz2

# Install confuse
cd distr
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
yum install -y cmake gcc-c++ unzip

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
