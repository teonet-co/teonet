#!/bin/sh
# 
# File:   make_remote_doc_upload.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Make doxygen documentation and upload it to web-site (repository) by FTP
#
# Created on Sep 3, 2015, 1:02:36 AM
#

# Parameters:
# @param $1 PROJECT_NAME (default: teonet)
# Usage example:
# sh/make_remote_doc_upload teonet

# Check parameters
if [ -z "$1" ];  then
    PROJECT_NAME="teonet";
else
    PROJECT_NAME=$1
fi

# Exit with error if GitLab CI variable is not set
if [ -z $CI_TEONET_REPO_FTP_PWD ]; then
    echo "Can't upload documentation from local host"
    exit 1
    #CI_TEONET_REPO_FTP_PWD="VV9x5ClC"
fi

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

DOC_MAKE="make doxygen-doc"
DOC_FOLDER=docs/html

REPO_DOC_FOLDER=/docs/$PROJECT_NAME

# Make documentation
echo $ANSI_BROWN"Make documentation:"$ANSI_NONE
echo ""
$DOC_MAKE
echo ""

# Upload project documentation to remote host
echo $ANSI_BROWN"Upload project documentation to remote host:"$ANSI_NONE
echo ""
lftp -c "
set ftp:list-options -a;
open ftp://$REPO_USER:$REPO_PASSWORD@repo2.ksproject.org; 
lcd $DOC_FOLDER;
cd $REPO_DOC_FOLDER;
mirror --reverse --delete --use-cache --verbose --allow-chown --only-newer --ignore-time
"
echo ""
