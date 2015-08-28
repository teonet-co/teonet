#!/bin/sh
# 
# File:   make_deb_remote_copy.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Upload (mirror) local repository to remote host
#
# Created on Aug 27, 2015, 2:08:45 AM
#

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

REPO=../repo

# Upload local repository to remote host
echo $ANSI_BROWN"Upload local repository to remote host:"$ANSI_NONE
echo ""
sudo apt-get install -y lftp
lftp -c "
set ftp:list-options -a;
open ftp://repo:VV9x5ClC@repo.ksproject.org; 
if [ -z "$1" ]
  then

    lcd $REPO/ubuntu;
    cd /ubuntu;

  else

    lcd $REPO/rhel;
    cd /rhel;
fi
mirror --reverse --delete --use-cache --verbose --allow-chown
"
echo ""
