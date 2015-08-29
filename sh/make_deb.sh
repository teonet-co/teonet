#!/bin/sh
# Create DEBIAN package

# Parameters:
#
# @param $1 Version
# @param $2 Release 
# @param $3 Architecture
# @param $4 RPM subtype

set -e # exit at error

# The first parameter is required
if [ -z "$1" ]
  then
    exit -1
fi

VER_ONLY=$1
if [ -z "$2" ]
  then
    RELEASE=1
  else
    RELEASE=$2
fi
if [ -z "$3" ]
  then
    ARCH="amd64"
  else
    ARCH=$3
fi
VER=$1-$RELEASE


VER_ARCH=$VER"_"$ARCH
PWD=`pwd`
REPO=../repo

PACKET_NAME="libteonet"
PACKET_DESCRIPTION="Teonet library version $VER
 Mesh network library."

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

echo $ANSI_BROWN"Create debian packet $PACKET_NAME""_$VER_ARCH.deb"$ANSI_NONE
echo ""

#Update and upgrade build host
#echo $ANSI_BROWN"Update and upgrade build host:"$ANSI_NONE
#echo ""
#sudo apt-get update
#sudo apt-get -y upgrade
#echo ""

# Configure and make
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

# Install to temporary folder 
echo $ANSI_BROWN"Make install to temporary folder:"$ANSI_NONE
echo ""
make install DESTDIR=$PWD/$PACKET_NAME"_"$VER_ARCH
echo ""

# Create DEBIAN control file
echo $ANSI_BROWN"Create DEBIAN control file:"$ANSI_NONE
echo ""
# Note: Add this to Depends if test will be added to distributive: 
# libcunit1-dev (>= 2.1-2.dfsg-1)
mkdir $PACKET_NAME"_"$VER_ARCH/DEBIAN
cat << EOF > $PACKET_NAME"_"$VER_ARCH/DEBIAN/control
Package: $PACKET_NAME
Version: $VER
Section: libdevel
Priority: optional
Architecture: $ARCH
Depends: libssl-dev (>= 1.0.1f-1ubuntu2.15), libev-dev (>= 4.15-3), libconfuse-dev (>= 2.7-4ubuntu1), uuid-dev (>= 2.20.1-5.1ubuntu20.4)
Maintainer: Kirill Scherba <kirill@scherba.ru>
Description: $PACKET_DESCRIPTION

EOF

# Build package
echo $ANSI_BROWN"Build package:"$ANSI_NONE
echo ""
if [ -f $PACKET_NAME"_"$VER_ARCH".deb" ]
then
    rm $PACKET_NAME"_"$VER_ARCH.deb
fi
dpkg-deb --build $PACKET_NAME"_"$VER_ARCH
rm -rf $PACKET_NAME"_"$VER_ARCH
echo ""

# Install and run application to check created package
echo $ANSI_BROWN"Install package and run application to check created package:"$ANSI_NONE
echo ""
set +e
sudo dpkg -i $PACKET_NAME"_"$VER_ARCH.deb
set -e
sudo apt-get install -y -f
echo ""
echo $ANSI_BROWN"Run application:"$ANSI_NONE
echo ""
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
sudo apt-get remove -y $PACKET_NAME
sudo apt-get autoremove -y
echo ""

# Add packet to repository ----------------------------------------------------

# Install reprepro
echo $ANSI_BROWN"Install reprepro:"$ANSI_NONE
echo ""
sudo apt-get install -y reprepro
echo ""

# Create working directory
if [ ! -d "$REPO" ];   then
    mkdir $REPO
fi

REPO="$REPO/ubuntu"

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

# Upload repository to remote host and Test Install and run application
if [ ! -z "$CI_BUILD_REF" ]; then
    
    # Upload repository to remote host
    # by ftp: 
    sh/make_remote_upload.sh
    if [ "$?" = "0" ]; then
    else
        exit 1
    fi

    # Install packet from remote repository
    sh/make_remote_install.sh
    if [ "$?" = "0" ]; then
    else
        exit 1
    fi

fi
