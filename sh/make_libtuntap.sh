#!/bin/sh
# 
# File:   make_libtuntap.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Build libtuntap DEB / RPM
#
# Created on Aug 30, 2015, 12:43:22 AM
#

# Include make deb functions
PWD=`pwd`
#echo "include: $PWD/sh/make_deb_inc.sh"
. "$PWD/sh/make_deb_inc.sh"

# Parameters
# @param $1 Architecture
# @param $2 RPM subtype rpm yum zyp

# Check param
if [ -z "$2" ]; then
    # Default - UBUNTU
    RPM_SUBTYPE="deb"
    INST="sudo apt-get install -y "
    RPM_DEV="rpm"
  else
    # Default - UBUNTU
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

#echo "Parameters:"
#echo "ARCH=$ARCH"
#echo "RPM_SUBTYPE=$RPM_SUBTYPE"
#echo ""

VER=0.3.0
RELEASE=1
PACKET_NAME=libtuntap-dev
DEPENDS=""
MAINTAINER="Kirill Scherba <kirill@scherba.ru>"
PACKET_DESCRIPTION="libtuntap is a library for configuring TUN or TAP devices in a portable manner."

PWD=`pwd`
REPO=../../repo # Repository folder
REPO_DEB=ubuntu # UBUNTU/DEBIAN sub-folder
REPO_RPM=rhel # Rehl sub-folder
RPMBUILD=~/rpmbuild # RPM Build folder 

# Main message
echo $ANSI_BROWN"Create $PACKET_NAME package and add it to local repository"$ANSI_NONE
echo ""

# Install libtuntap dependence
echo $ANSI_BROWN"Install libtuntap dependence"$ANSI_NONE
echo ""
if [ $RPM_SUBTYPE = "deb" ]; then
    sudo apt-get install -y cmake g++ unzip
    VER_ARCH=$VER"_"$ARCH
    DIV="_"
    PACKAGE_NAME=$PACKET_NAME$DIV$VER_ARCH
else
    yum install -y cmake gcc-c++ unzip
    VER_ARCH=$VER"."$ARCH
    DIV="-"
    PACKAGE_NAME=$PACKET_NAME$DIV$VER
fi
echo ""

# Get and build libtuntap
echo $ANSI_BROWN"Get, make and install libtuntap"$ANSI_NONE
echo ""

# Get libtuntap
unzip distr/libtuntap.zip
cd libtuntap-master

# Libtuntap build
cmake ./
make
make install DESTDIR=$PWD/$PACKAGE_NAME
echo ""

# Create package
if [ $RPM_SUBTYPE = "deb" ]; then

    # Create DEBIAN control file
    create_deb_control $PACKAGE_NAME $PACKET_NAME $VER $ARCH $DEPENDS $MAINTAINER $PACKET_DESCRIPTION

    # Build package
    build_deb_package $PACKAGE_NAME

    # Add DEB packages to local repository
    add_deb_package $REPO/$REPO_DEB teonet $PACKAGE_NAME

else

    # Create binary tarball
    tar -zcvf $PACKET_NAME$DIV$VER.tar.gz $PACKET_NAME$DIV$VER/
    rm -rf $PACKET_NAME$DIV$VER/
    mv -f  $PACKET_NAME$DIV$VER.tar.gz $RPMBUILD/SOURCES

fi

cd ..
rm -fr libtuntap-master
echo ""
