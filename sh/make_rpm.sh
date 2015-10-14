#!/bin/sh
#
# File:   make_rpm.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Created on Aug 28, 2015, 1:47:35 AM
#

# Parameters:
#
# @param $1 Version
# @param $2 Library HI version
# @param $3 Library version
# @param $4 Release
# @param $5 Architecture
# @param $6 RPM subtype rpm yum zyp
# @param $7 PACKET_NAME
# @param $8 PACKET_SUMMARY

# Include make RPM functions
PWD=`pwd`
. "$PWD/sh/make_rpm_inc.sh"

# Set exit at error
set -e

# Check parameters and set defaults
check_param $1 $2 $3 $4 $5 $6 $7 $8
# Set global variables:
# VER=$1
# RELEASE=$2
# LIBRARY_HI_VERSION=$3
# LIBRARY_VERSION=$4
# ARCH=$5
# INST="yum install -y "
# RPM_SUBTYPE="yum"
# RPM_DEV="rpm-build"
# VER=$1-$RELEASE
# PACKET_NAME=$7
# PACKET_DESCRIPTION=$8


#echo "Show params: \n1=$1\n2=$2\n3=$3\n4=$4\n"
#echo "RPM_SUBTYPE="$RPM_SUBTYPE
#echo "INST="$INST
#echo "RPM_DEV="$RPM_DEV
#echo ""
#
#exit 2

REPO=../repo
VER_ARCH=$VER"_"$ARCH
RPMBUILD=~/rpmbuild
PACKAGE_NAME=$PACKET_NAME"-"$VER

#ANSI_BROWN="\033[22;33m"
#ANSI_NONE="\033[0m"

echo $ANSI_BROWN"Create RPM packet $PACKET_NAME""_$VER_ARCH"$ANSI_NONE
echo ""

# Update and upgrade build host
update_host

# Create your RPM build environment for RPM
set_rpmbuild_env $RPMBUILD

# Create temporary folder to install files and build tarball from it
mkdir $PACKET_NAME-$VER

# Configure and make auto configure project (in current folder)
make_counfigure

# Make install
make_install $PWD/$PACKAGE_NAME

# Create binary tarball
build_rpm_tarball $PACKAGE_NAME

# Copy tarball to the sources folder and create spec file
RPM_FILES="/usr/bin/teovpn
   /usr/bin/teodb
   /usr/bin/teoweb
   /usr/doc/teonet/
   /usr/etc/teonet/teonet.conf.default
   /usr/include/teonet/
   /usr/lib/libteonet.a
   /usr/lib/libteonet.la
   /usr/lib/libteonet.so
   /usr/lib/libteonet.so.$LIBRARY_HI_VERSION
   /usr/lib/libteonet.so.$LIBRARY_VERSION
   /usr/share/doc/teonet/examples/
   /usr/share/man/man3/
   /usr/include/teol0/teonet_l0_client.h
   /usr/lib/libteol0.a
   /usr/lib/libteol0.la
   /usr/lib/libteol0.so
   /usr/lib/libteol0.so.1
   /usr/lib/libteol0.so.1.0.0  
"
create_rpm_control $RPMBUILD $PACKAGE_NAME $PACKET_NAME $VER $RELEASE "${PACKET_SUMMARY}" "${RPM_FILES}"

# Build the source and the binary RPM
build_rpm "${INST}$RPM_DEV" $RPMBUILD $PACKET_NAME

# Add dependences to the repository
#if [ $REPO_JUST_CREATED = 1 ]; then

    # Make and add libtuntap
    sh/make_libtuntap.sh $RPM_SUBTYPE

#fi

# Create RPM repository and add packages to this repository
if [ $RPM_SUBTYPE = 'zyp' ]; then
    SUBFOLDER="opensuse"
else
    SUBFOLDER="rhel"
fi
create_rpm_repo $RPMBUILD $REPO/$SUBFOLDER $ARCH "${INST}"

# Upload repository to remote host and Test Install and run application
if [ ! -z "$CI_BUILD_REF" ]; then

    # Upload repository to remote host by ftp:
    sh/make_remote_upload.sh $RPM_SUBTYPE "$INST"

    # Install packet from remote repository
    sh/make_remote_install.sh $RPM_SUBTYPE "$INST"

fi
