#!/bin/sh

# Configure and make
./configure --prefix=/usr
make

# Install
make install DESTDIR=/root/Project/teonet_build/libteonet-0.0.7

# Create DEBIAN control file
mkdir libteonet-0.0.7/DEBIAN
sh/make_deb_control.sh > libteonet-0.0.7/DEBIAN/control

# Build package
if [ -f "libteonet-0.0.7.deb" ]
then
    rm libteonet-0.0.7.deb
fi
dpkg-deb --build libteonet-0.0.7
rm -rf libteonet-0.0.7

# Imstall & remove package to check created package
dpkg -i libteonet-0.0.7.deb
apt-get install -f
teovpn
apt-get remove -y libteonet
