#!/bin/sh
# 
# File:   make_libtuntap.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Build libtuntap DEB / RPM
#
# Created on Aug 30, 2015, 12:43:22 AM
#

# Parameters
# @param $1 Architecture
# @param $2 RPM subtype rpm yum zyp

if [ -z "$2" ]; then
    # Default - Ubuntu
    RPM_SUBTYPE="deb"
    INST="sudo apt-get install -y "
    RPM_DEV="rpm"
  else
    # Default - Ubuntu
    RPM_SUBTYPE=$2
    INST="sudo apt-get install -y "
    RPM_DEV="rpm"
    # Rehl
    if [ "$RPM_SUBTYPE" = "yum" ]; then
        INST="yum install -y "
        RPM_DEV="rpm-build"
    fi
    # Suse
    if [ "$RPM_SUBTYPE" = "zyp" ]; then
        INST="zypper install -y "
        RPM_DEV="rpm-devel"
    fi
fi
if [ -z "$1" ]; then
  if [ $RPM_SUBTYPE = "deb" ]; then
    ARCH="amd64"
  else
    ARCH="x86_64"
  fi
  else
    ARCH=$3
fi

echo "Parameters:"
echo "ARCH=$ARCH"
echo "RPM_SUBTYPE=$RPM_SUBTYPE"

VER=0.3.0
RELEASE=1
PACKET_NAME=libtuntap
PACKET_DESCRIPTION="libtuntap is a library for configuring TUN or TAP devices in a portable manner."

PWD=`pwd`
REPO_DEB=../../repo/ubuntu
REPO_RPM=../../repo/rhel
RPMBUILD=~/rpmbuild

# Install libtuntap dependence
if [ $RPM_SUBTYPE = "deb" ]; then
    sudo apt-get install -y cmake g++ unzip
    VER_ARCH=$VER"_"$ARCH
    DIV="_"
else
    yum install -y cmake gcc-c++ unzip
    VER_ARCH=$VER"."$ARCH
    DIV="-"
fi

# Get libtuntap
unzip distr/libtuntap.zip
cd libtuntap-master

# Libtuntap build
cmake ./
make
if [ $RPM_SUBTYPE = "deb" ]; then
    make install DESTDIR=$PWD/$PACKET_NAME$DIV$VER_ARCH
else
    make install DESTDIR=$PWD/$PACKET_NAME$DIV$VER
fi

if [ $RPM_SUBTYPE = "deb" ]; then
    # Create DEBIAN control file
    # Note: Add this to Depends if test will be added to distributive: 
    mkdir $PACKET_NAME$DIV$VER_ARCH/DEBIAN
    cat << EOF > $PACKET_NAME$DIV$VER_ARCH/DEBIAN/control
Package: $PACKET_NAME
Version: $VER
Section: libdevel
Priority: optional
Architecture: $ARCH
Depends: 
Maintainer: Kirill Scherba <kirill@scherba.ru>
Description: $PACKET_DESCRIPTION

EOF
    dpkg-deb --build $PACKET_NAME$DIV$VER_ARCH
    rm -rf $PACKET_NAME$DIV$VER_ARCH
    reprepro --ask-passphrase -Vb $REPO_DEB includedeb teonet $PACKET_NAME$DIV$VER_ARCH.deb
else
    # Create binary tarball
    tar -zcvf $PACKET_NAME$DIV$VER.tar.gz $PACKET_NAME$DIV$VER/
    rm -rf $PACKET_NAME$DIV$VER/
    mv -f  $PACKET_NAME$DIV$VER.tar.gz $RPMBUILD/SOURCES
fi

cd ..
#rm -fr libtuntap-master
