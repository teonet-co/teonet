#!/bin/sh

# Install dependences
emerge doxygen libev cunit confuse

# Get libtuntap
cd distr
unzip libtuntap.zip
cd libtuntap-master

# Libtuntap build
cmake ./
make
make install

# Remove libtuntap source
cd ..
rm -fr libtuntap-master
cd ..
