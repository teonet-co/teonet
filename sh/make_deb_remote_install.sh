#!/bin/sh
# 
# File:   make_deb_remote_install.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Install libteonet from remote repository
#
# Created on Aug 27, 2015, 1:39:09 AM
#

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

# Install libteonet from remote repository
echo $ANSI_BROWN"Install libteonet from remote repository:"$ANSI_NONE
echo ""

# Get key registered at: http://pgp.mit.edu/
# wget -O - http://repo.ksproject.org/ubuntu/key/deb.gpg.key | sudo apt-key add -
# or
echo $ANSI_BROWN"Get key registered at: http://pgp.mit.edu/:"$ANSI_NONE
echo ""
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8CC88F3BE7D6113C
echo ""

# Add repository
echo $ANSI_BROWN"Add repository:"$ANSI_NONE
echo ""
sudo apt-get install -y software-properties-common
sudo add-apt-repository "deb http://repo.ksproject.org/ubuntu/ teonet main"
sudo apt-get update
echo ""

# Install Teonet library from remote repository
echo $ANSI_BROWN"Install Teonet library from remote repository:"$ANSI_NONE
echo ""
sudo apt-get install -y libteonet
echo ""

# Run application
echo $ANSI_BROWN"Run application:"$ANSI_NONE
echo ""
teovpn -?
echo ""

# Remove Teonet library from host
echo $ANSI_BROWN"Remove Teonet library from host:"$ANSI_NONE
echo ""
sudo apt-get remove -y libteonet
sudo apt-get autoremove -y
echo ""

# TODO: Remove teonet repository & key
# Edit the sources list (to remove teonet repository)
# sudo mcedit /etc/apt/sources.list
