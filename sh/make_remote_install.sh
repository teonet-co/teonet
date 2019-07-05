#!/bin/sh
# 
# File:   make_remote_install.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Install libteonet from remote repository
#
# Created on Aug 27, 2015, 1:39:09 AM
#

set -e # exit at error

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

# Parameters:
#
# @param $1 RPM_SUBTYPE: deb, rpm, yum, zyp (default: deb)
# @param $2 Install prefix: default: sudo apt-get install -y 
#

# Check parameters
if [ -z "$1" ];  then
    RPM_SUBTYPE="deb";
else
    RPM_SUBTYPE=$1
fi
if [ -z "$2" ];  then
    sudo apt-get install -y lftp
else
    $2"lftp"
fi
if [ -z "$3" ];  then
    if [ $RPM_SUBTYPE = "deb" ]; then
        PACKET_NAME="libteonet-dev"
    else
        PACKET_NAME="libteonet"
    fi
else
    PACKET_NAME=$3
fi

# Install libteonet from remote repository
echo $ANSI_BROWN"Install libteonet from remote repository:"$ANSI_NONE
echo ""

# Set repository key, add repository, install teonet library
if [ "$RPM_SUBTYPE" = "deb" ]; then
    # Get key registered at: http://pgp.mit.edu/
    # wget -O - http://repo.ksproject.org/ubuntu/key/deb.gpg.key | sudo apt-key add -
    # or
    echo $ANSI_BROWN"Set key registered at: http://pgp.mit.edu/:"$ANSI_NONE
    echo ""
    sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8CC88F3BE7D6113C
    echo ""

    # Add repository
    echo $ANSI_BROWN"Add repository:"$ANSI_NONE
    echo ""
    sudo apt-get install -y software-properties-common
    #sudo add-apt-repository "deb http://repo.ksproject.org/ubuntu/ teonet main"
    sudo add-apt-repository "deb http://repo2.ksproject.org/repo/ubuntu/ teonet main"
    sudo apt-get update
    echo ""

    # Install Teonet library from remote repository
    echo $ANSI_BROWN"Install Teonet library from remote repository:"$ANSI_NONE
    echo ""
    sudo apt-get install -y $PACKET_NAME
    echo ""
else 
    if [ "$RPM_SUBTYPE" = "zyp" ]; then
        zypper ar -f http://repo.ksproject.org/opensuse/x86_64/ teonet
        zypper in -y $PACKET_NAME
        ldconfig
    fi
    if [ "$RPM_SUBTYPE" = "yum" ]; then

        # Add repository
        cat <<EOF > /etc/yum.repos.d/teonet.repo
[teonet]
name=Teonet library for RHEL / CentOS
baseurl=http://repo.ksproject.org/rhel/x86_64/
enabled=1
gpgcheck=0
#gpgkey=http://foo.nixcraft.com/RPM-GPG-KEY.txt
EOF

        # Install Teonet library from remote repository
        yum install -y $PACKET_NAME
        ldconfig
    fi
fi

# Run application
echo $ANSI_BROWN"Run application:"$ANSI_NONE
echo ""
teovpn -?
echo ""

# Remove Teonet library from host
if [ "$RPM_SUBTYPE" = "deb" ]; then
    echo $ANSI_BROWN"Remove Teonet library from host:"$ANSI_NONE
    echo ""
    sudo apt-get remove -y $PACKET_NAME
    sudo apt-get autoremove -y
    echo ""
else
    if [ "$RPM_SUBTYPE" = "yum" ]; then
        yum remove -y $PACKET_NAME
    fi
fi

# TODO: Remove teonet repository & key
# Edit the sources list (to remove teonet repository)
# sudo mcedit /etc/apt/sources.list
