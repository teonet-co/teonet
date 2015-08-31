#!/bin/sh
# 
# File:   make_inc.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Created on Aug 31, 2015, 3:32:50 PM
#

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

# Configure and make auto configure project (in current folder)
make_counfigure() {
    echo $ANSI_BROWN"Configure or autogen:"$ANSI_NONE
    echo ""
    if [ -f "autogen.sh" ]
    then
    ./autogen.sh --prefix=/usr
    else
    ./configure --prefix=/usr
    fi
    echo ""
    echo $ANSI_BROWN"Make:"$ANSI_NONE
    echo ""
    make
    echo ""
}

# Make install project (in current folder) to folder
# Parameters:
# @param $1 Destination folder (should be absolute path)
make_install() {
    # Install to temporary folder 
    echo $ANSI_BROWN"Make install to temporary folder:"$ANSI_NONE
    echo ""
    make install DESTDIR=$1
    echo ""
}
