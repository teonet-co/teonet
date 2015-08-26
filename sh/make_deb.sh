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

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

echo $ANSI_BROWN"Create debian package libteonet_$VER_ARCH.deb"$ANSI_NONE
echo ""

#Update and upgrade build host
echo $ANSI_BROWN"Update and upgrade build host:"$ANSI_NONE
echo ""
sudo apt-get update
sudo apt-get -y upgrade
echo ""

# Configure and make
echo $ANSI_BROWN"Configure:"$ANSI_NONE
echo ""
./configure --prefix=/usr
echo ""
echo $ANSI_BROWN"Make:"$ANSI_NONE
echo ""
make
echo ""

# Install to temporary folder 
echo $ANSI_BROWN"Make install to temporary folder:"$ANSI_NONE
echo ""
make install DESTDIR=$PWD/libteonet_$VER_ARCH
echo ""

# Create DEBIAN control file
echo $ANSI_BROWN"Create DEBIAN control file:"$ANSI_NONE
echo ""
# Note: Add this to Depends if test will be added to distributive: 
# libcunit1-dev (>= 2.1-2.dfsg-1)
mkdir libteonet_$VER_ARCH/DEBIAN
cat << EOF > libteonet_$VER_ARCH/DEBIAN/control
Package: libteonet
Version: $VER
Section: libdevel
Priority: optional
Architecture: $ARCH
Depends: libssl-dev (>= 1.0.1f-1ubuntu2.15), libev-dev (>= 4.15-3), libconfuse-dev (>= 2.7-4ubuntu1), uuid-dev (>= 2.20.1-5.1ubuntu20.4)
Maintainer: Kirill Scherba <kirill@scherba.ru>
Description: Teonet library version $VER
 Mesh network library.

EOF

# Build package
echo $ANSI_BROWN"Build package:"$ANSI_NONE
if [ -f "libteonet_$VER_ARCH.deb" ]
then
    rm libteonet_$VER_ARCH.deb
fi
dpkg-deb --build libteonet_$VER_ARCH
rm -rf libteonet_$VER_ARCH
echo ""

# Install and run application to check created package
echo $ANSI_BROWN"Install package and run application to check created package:"$ANSI_NONE
echo ""
set +e
sudo dpkg -i libteonet_$VER_ARCH.deb
set -e
sudo apt-get install -y -f
teovpn -?
echo ""

# Show version of installed depends
echo $ANSI_BROWN"Show version of installed depends:"$ANSI_NONE
echo ""
echo "libssl-dev:"
dpkg -s libssl-dev | grep Version:
echo ""
echo "libev-dev:"
dpkg -s libev-dev | grep Version:
echo ""
echo "libconfuse-dev:"
dpkg -s libconfuse-dev | grep Version:
echo ""
echo "uuid-dev:"
dpkg -s uuid-dev | grep Version:
echo ""

# Remove package  
echo $ANSI_BROWN"Remove package:"$ANSI_NONE
echo ""
sudo apt-get remove -y libteonet
sudo apt-get autoremove -y
echo ""

# Add packet to repository ----------------------------------------------------

# Install reprepro
echo $ANSI_BROWN"Install reprepro:"$ANSI_NONE
echo ""
sudo apt-get install -y reprepro
echo ""

# Create working directory
if [ ! -d "repo" ];   then
    mkdir repo
fi

REPO="repo/ubuntu"

# Create repository and configuration files
if [ ! -d "$REPO" ]; then     
echo $ANSI_BROWN"Create repository and configuration files:"$ANSI_NONE
echo ""

mkdir $REPO
mkdir $REPO/conf 

cat << EOF > $REPO/conf/distributions
Origin: Teonet
Label: Teonet
Suite: stable
Codename: teonet
Version: 0.1
Architectures: i386 amd64 source
Components: main
Description: Teonet
SignWith: yes
EOF

cat << EOF > $REPO/conf/options

verbose
ask-passphrase
basedir .

EOF

# Import repository keys to this host
echo "Before Import repository keys..."
str=`gpg --list-keys | grep "repository <repo@ksproject.org>"`
if [ -z "$str" ]; then 
  echo $ANSI_BROWN"Add repository keys to this host:"$ANSI_NONE
  gpg --allow-secret-key-import --import sh/gpg_key/deb-sec.gpg.key
  gpg --import sh/gpg_key/deb.gpg.key
fi
echo "After Import repository keys..."

# Remove keys from this host
# gpg --delete-secret-key  "repository <repo@ksproject.org>"
# gpg --delete-key  "repository <repo@ksproject.org>"

# Export key to repository folder
mkdir $REPO/key
gpg --armor --export repository repo@ksproject.org >> $REPO/key/deb.gpg.key
# gpg --armor --export-secret-key repository repo@ksproject.org >> $REPO/key/deb-sec.gpg.key

# Create the repository tree
reprepro --ask-passphrase -Vb $REPO export

echo ""
fi

# Add DEB packages to local repository
echo $ANSI_BROWN"Add DEB package to local repository:"$ANSI_NONE
echo ""
reprepro --ask-passphrase -Vb $REPO includedeb teonet *.deb
echo ""

# Add repository to this host
#
# The key registered at: http://pgp.mit.edu/
# wget -O - http://repo.ksproject.org/ubuntu/key/deb.gpg.key | sudo apt-key add -
# or
# sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8CC88F3BE7D6113C
#
# sudo apt-get install -y software-properties-common
# sudo add-apt-repository "deb http://repo.ksproject.org/ubuntu/ teonet main"
# sudo apt-get update
#

# Install Teonet library from repository
# sudo apt-get install -y libteonet
#

# Run application
# teovpn -?

# Remove Teonet library from repository
# sudo apt-get remove -y libteonet
# sudo apt-get autoremove -y
#

# TODO: Remove teonet repository & key
# Edit the sources list (to remove teonet repository)
# sudo mcedit /etc/apt/sources.list

