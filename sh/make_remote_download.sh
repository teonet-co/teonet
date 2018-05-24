#!/bin/sh
# 
# File:   make_remote_download.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Upload (mirror) local repository to remote host
#
# Created on Aug 27, 2015, 2:08:45 AM
#

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

REPO=../repo

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
if [ "$RPM_SUBTYPE" = "deb" ]; then
  SUBFOLDER=ubuntu
else 
    if [ "$RPM_SUBTYPE" = "zyp" ]; then
      SUBFOLDER=opensuse
    else
      SUBFOLDER=rhel
    fi
fi

mkdir $REPO
mkdir $REPO/$SUBFOLDER

# Upload local repository to remote host
echo $ANSI_BROWN"Download remote host to local repository:"$ANSI_NONE
echo ""
lftp -c "
set ftp:list-options -a;
open ftp://repo:$CI_TEONET_REPO_FTP_PWD@repo.ksproject.org; 
lcd $REPO/$SUBFOLDER;
cd /$SUBFOLDER;
mirror --delete --use-cache --verbose --allow-chown
"
echo ""
