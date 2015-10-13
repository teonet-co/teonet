#!/bin/sh

rm teonet_l0_client.py teonet_l0_client_wrap.c

swig -python teonet_l0_client.i

gcc -c teonet_l0_client.c teonet_l0_client_wrap.c \
    -I/usr/include/python2.7 -fPIC

ld -shared teonet_l0_client.o teonet_l0_client_wrap.o -o _teonet_l0_client.so

# gcc -dynamiclib teonet_l0_client.o teonet_l0_client_wrap.o -o _teonet_l0_client.dylib

cp _teonet_l0_client.so ../../examples
