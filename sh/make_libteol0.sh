#!/bin/sh
# 
# File:   make_libteol0.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Build libteol0 DEB / RPM
#
# Created on Oct 12, 2015, 19:53:12 AM
#

# Parameters
# @param $1 RPM subtype rpm yum zyp
# @param $2 Architecture

# Check param
if [ -z "$1" ]; then
    # Default - UBUNTU
    RPM_SUBTYPE="deb"
    INST="sudo apt-get install -y "
    RPM_DEV="rpm"
  else
    # Default - UBUNTU
    RPM_SUBTYPE=$1
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
if [ -z "$2" ]; then
  if [ $RPM_SUBTYPE = "deb" ]; then
    ARCH="amd64"
  else
    ARCH="x86_64"
  fi
  else
    ARCH=$2
fi

# Include make deb or rpm functions
PWD=`pwd`
if [ $RPM_SUBTYPE = "deb" ]; then
. "$PWD/sh/make_deb_inc.sh"
else
. "$PWD/sh/make_rpm_inc.sh"
fi

#echo "Parameters:"
#echo "ARCH=$ARCH"
#echo "RPM_SUBTYPE=$RPM_SUBTYPE"
#echo ""

VER=0.0.1       
RELEASE=1
if [ $RPM_SUBTYPE = "deb" ]; then
    PACKET_NAME=libteol0-dev
else
    PACKET_NAME=libteol0
fi
DEPENDS=""
MAINTAINER="Kirill Scherba <kirill@scherba.ru>"
PACKET_DESCRIPTION="libteol0 is a library for connecting to Teonet L0 server."
PACKET_SUMMARY=$PACKET_DESCRIPTION

PWD=`pwd`
REPO=../../../repo # Repository folder
REPO_DEB=ubuntu # UBUNTU/DEBIAN sub-folder
#REPO_RPM=rhel # Rehl sub-folder
RPMBUILD=~/rpmbuild # RPM Build folder 

# Main message
echo $ANSI_BROWN"Create $PACKET_NAME package and add it to local repository"$ANSI_NONE
echo ""

# Install libteol0 dependence
echo $ANSI_BROWN"Install libteol0 dependence"$ANSI_NONE
echo ""
if [ $RPM_SUBTYPE = "deb" ] || [ $RPM_SUBTYPE = "rpm" ]; then
    #$INST"cmake g++ unzip"
    VER_ARCH=$VER"_"$ARCH
    if [ $RPM_SUBTYPE = "deb" ]; then
        DIV="_"
        PACKAGE_NAME=$PACKET_NAME$DIV$VER_ARCH
    else
        DIV="-"
        PACKAGE_NAME=$PACKET_NAME$DIV$VER
    fi
else
    #$INST"cmake gcc-c++ unzip"
    VER_ARCH=$VER"."$ARCH
    DIV="-"
    PACKAGE_NAME=$PACKET_NAME$DIV$VER
fi
echo ""

# Get and build libteol0
echo $ANSI_BROWN"Get, make and install libteol0"$ANSI_NONE
echo ""

# Get libteol0
#unzip distr/libtuntap.zip
cd embedded/libteol0

# Libteol0 build
#cmake ./
make
make install DESTDIR=$PWD/$PACKAGE_NAME
echo ""

# Create package
if [ $RPM_SUBTYPE = "deb" ]; then

    # Create DEBIAN control file
    create_deb_control $PACKAGE_NAME $PACKET_NAME $VER $ARCH "${DEPENDS}" "${MAINTAINER}" "${PACKET_DESCRIPTION}"

    # Build package
    build_deb_package $PACKAGE_NAME

    # Add DEB packages to local repository
    add_deb_package $REPO/$REPO_DEB teonet $PACKAGE_NAME

else

    # Create binary tarball
    build_rpm_tarball $PACKAGE_NAME

    # Copy tarball to the sources folder and create spec file
    RPM_FILES="/usr/include/teol0/teonet_l0_client.h
/usr/lib/libteol0.a
/usr/lib/libteol0.la
/usr/lib/libteol0.so
/usr/lib/libteol0.so.1
/usr/lib/libteol0.so.1.0.0"
    create_rpm_control $RPMBUILD $PACKAGE_NAME $PACKET_NAME $VER $RELEASE "${PACKET_SUMMARY}" "${RPM_FILES}"

    # Build the source and the binary RPM
    build_rpm "${INST}$RPM_DEV" $RPMBUILD $PACKET_NAME

fi

cd ../..
#rm -fr libtuntap-master
echo ""
