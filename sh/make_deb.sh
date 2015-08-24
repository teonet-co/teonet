#!/bin/sh
# Create DEBIAN package

set -e

VER=$1
if [ -z "$2" ]
  then
    ARCH="amd64"
  else
    ARCH=$2
fi
VER_ARCH=$VER"_"$ARCH
PWD=`pwd`

#echo $VER_ARCH
#exit

sudo apt-get update
sudo apt-get -y upgrade

echo Create debian package libteonet-$VER_ARCH.deb

# Configure and make
./configure --prefix=/usr
make

# Install to temporary folder 
make install DESTDIR=$PWD/libteonet-$VER_ARCH

# Create DEBIAN control file
# Note: Add this to Depends if test will be added to distributive: 
# libcunit1-dev (>= 2.1-2.dfsg-1)
mkdir libteonet-$VER_ARCH/DEBIAN
cat << EOF > libteonet-$VER_ARCH/DEBIAN/control
Package: libteonet
Version: $VER
Section: libdevel
Priority: optional
Architecture: $ARCH
Depends: libssl-dev (>= 1.0.1f-1ubuntu2.15), libev-dev (>= 4.15-3), libconfuse-dev (>= 2.7-4ubuntu1), uuid-dev (>= 2.20.1-5.1ubuntu20.4)
Maintainer: Kirill Scherba <kirill@scherba.ru>
Description: Teonet library
 Mesh network library.

EOF

# Build package
if [ -f "libteonet-$VER_ARCH.deb" ]
then
    rm libteonet-$VER_ARCH.deb
fi
dpkg-deb --build libteonet-$VER_ARCH
rm -rf libteonet-$VER_ARCH

# Install, run application & to check created package
set +e
sudo dpkg -i libteonet-$VER_ARCH.deb
set -e
sudo apt-get install -y -f
teovpn -?

# Show version of installed depends
echo "Version of depends:"
echo " "
echo "libssl-dev:"
dpkg -s libssl-dev | grep Version:
echo " "
echo "libev-dev:"
dpkg -s libev-dev | grep Version:
echo " "
echo "libconfuse-dev:"
dpkg -s libconfuse-dev | grep Version:
echo " "
echo "uuid-dev:"
dpkg -s uuid-dev | grep Version:
echo " "

# Remove package  
sudo apt-get remove -y libteonet
sudo apt-get autoremove -y

# Create repository files
sudo apt-get install -y reprepro
mkdir repo
mkdir repo/conf
mv libteonet-$VER_ARCH.deb repo
cat << EOF > conf/distributions
Origin: Teonet
Label: Teonet
Suite: stable
Codename: teonet
Version: 0.1
Architectures: amd64
Components: contrib
Description: Teonet
EOF
find ./repo -name \*.deb -exec reprepro -Vb repo includedeb teonet {} \;
