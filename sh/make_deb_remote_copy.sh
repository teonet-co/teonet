#!/bin/sh
# 
# File:   make_deb_remote_copy.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Upload (mirror) local repository to remote host
#
# Created on Aug 27, 2015, 2:08:45 AM
#

REPO=../repo

# Upload local repository to remote host
echo $ANSI_BROWN"Upload local repository to remote host:"$ANSI_NONE
echo ""
sudo apt-get install -y lftp
lftp -c "
set ftp:list-options -a;
open ftp://repo:VV9x5ClC@repo.ksproject.org; 
lcd $REPO/ubuntu;
cd /ubuntu;
mirror --reverse --delete --use-cache --verbose --allow-chown
"
echo ""