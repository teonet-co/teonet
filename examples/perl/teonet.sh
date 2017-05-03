#!/bin/sh
# 
# File:   teonet.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Created on May 3, 2017, 2:17:21 AM
#

swig -perl teonet.i
gcc -fpic -c `perl -MExtUtils::Embed -e ccopts -e ldopts` -I../../../src -I../../../embedded/libpbl/src -I../../../embedded/teocli/libteol0/ -I../../../embedded/libtrudp/src teonet_wrap.c
gcc -shared teonet_wrap.o -lteonet -o teonet.so

