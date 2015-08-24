#!/bin/sh
# Create DEBIAN package

VER=$1
PWD=`pwd`

echo Create debian package libteonet-$VER.deb

# Configure and make
./configure --prefix=/usr
make

# Install to temporary folder 
make install DESTDIR=$PWD/libteonet-$VER

# Create DEBIAN control file
mkdir libteonet-$VER/DEBIAN
sh/make_deb_control.sh $VER > libteonet-$VER/DEBIAN/control

# Build package
if [ -f "libteonet-$VER.deb" ]
then
    rm libteonet-$VER.deb
fi
dpkg-deb --build libteonet-$VER
rm -rf libteonet-$VER

# Install & remove package to check created package
sudo dpkg -i libteonet-$VER.deb
sudo apt-get install -f
teovpn
sudo apt-get remove -y libteonet
