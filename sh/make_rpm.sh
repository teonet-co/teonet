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
check_param $1 $2 $3 $4 $5 $6
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
create_rpm_control $RPMBUILD $PACKAGE_NAME $PACKET_NAME $VER $RELEASE "${PACKET_SUMMARY}"

# Build the source and the binary RPM
build_rpm "${INST}$RPM_DEV" $RPMBUILD $PACKET_NAME

# TODO: Add dependences to the repository
#if [ $REPO_JUST_CREATED = 1 ]; then

    # Make and add libtuntap
    sh/make_libtuntap.sh $RPM_SUBTYPE

#fi

# Create RPM repository and add packages to this repository
create_rpm_repo $RPMBUILD $REPO/rhel $ARCH "${INST}"

# Upload repository to remote host and Test Install and run application
if [ ! -z "$CI_BUILD_REF" ]; then
    
    # Upload repository to remote host by ftp: 
    sh/make_remote_upload.sh $RPM_SUBTYPE "$INST"

    # Install packet from remote repository
    sh/make_remote_install.sh $RPM_SUBTYPE "$INST"

fi
