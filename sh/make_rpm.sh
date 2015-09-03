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
# @param $2 Release 
# @param $3 Architecture
# @param $4 RPM subtype rpm yum zyp
# @param $5 PACKET_NAME
# @param $6 PACKET_SUMMARY

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
# ARCH=$3
# INST="yum install -y "
# RPM_SUBTYPE="yum"
# RPM_DEV="rpm-build"
# VER=$1-$RELEASE
# PACKET_NAME=$5
# PACKET_DESCRIPTION=$6
# LIBRARY_HI_VERSION=$7
# LIBRARY_VERSION=$8


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
   /usr/doc/teonet/AUTHORS
   /usr/doc/teonet/COPYING
   /usr/doc/teonet/ChangeLog
   /usr/doc/teonet/INSTALL
   /usr/doc/teonet/NEWS
   /usr/doc/teonet/README
   /usr/etc/teonet/teonet.conf.default
   /usr/include/teonet/config/conf.h
   /usr/include/teonet/config/config.h
   /usr/include/teonet/config/opt.h
   /usr/include/teonet/crypt.h
   /usr/include/teonet/ev_mgr.h
   /usr/include/teonet/hotkeys.h
   /usr/include/teonet/modules/net_cli.h
   /usr/include/teonet/modules/net_tcp.h
   /usr/include/teonet/modules/net_term.h
   /usr/include/teonet/modules/net_tun.h
   /usr/include/teonet/modules/vpn.h
   /usr/include/teonet/net_arp.h
   /usr/include/teonet/net_com.h
   /usr/include/teonet/net_core.h
   /usr/include/teonet/net_multi.h
   /usr/include/teonet/net_split.h
   /usr/include/teonet/pbl.h
   /usr/include/teonet/teonet.h
   /usr/include/teonet/utils/rlutil.h
   /usr/include/teonet/utils/string_arr.h
   /usr/include/teonet/utils/utils.h
   /usr/lib/libteonet.a
   /usr/lib/libteonet.la
   /usr/lib/libteonet.so
   /usr/lib/libteonet.so.$LIBRARY_HI_VERSION
   /usr/lib/libteonet.so.$LIBRARY_VERSION"
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
