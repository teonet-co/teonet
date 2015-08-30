#!/bin/sh
# 
# File:   make_deb_inc.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Bash functions to build sources, create DEB package add it to repository
#
# Created on Aug 31, 2015, 1:06:27 AM
#

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

# Function ---------------------------------------------------------------------

# Check parameters and set defaults (specific for libteonet
# Parameters:
# @param $1 Version (required)
# @param $2 Release (default 1)
# @param $3 Architecture (default and64)
# @param $4 Reserved
# @param $5 Package name (default libteonet)
# @param $6 Package description (default ...)
# Set global variables:
# VER_ONLY=$1
# RELEASE=$2
# ARCH=$3
# VER=$1-$RELEASE
# PACKET_NAME=$5
# PACKET_DESCRIPTION=$6
check_param()
{
    # The first parameter is required
    if [ -z "$1" ]; then
        echo The first parameter is required
        exit 1
    fi
    VER_ONLY=$1
    if [ -z "$2" ]; then
        RELEASE=1
      else
        RELEASE=$2
    fi
    if [ -z "$3" ]; then
        ARCH="amd64"
      else
        ARCH=$3
    fi
    VER=$1-$RELEASE
    if [ -z "$5" ]; then
        PACKET_NAME="libteonet-dev"
    else
        PACKET_NAME=$5
    fi
    if [ -z "$6" ]; then
        PACKET_DESCRIPTION="Teonet library version $VER
     Mesh network library."
    else
        PACKET_DESCRIPTION=$6
    fi
}

# Update and upgrade build host
update_host()
{
    #echo $ANSI_BROWN"Update and upgrade build host:"$ANSI_NONE
    #echo ""
    #sudo apt-get update
    #sudo apt-get -y upgrade
    echo ""
}

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

# Create DEBIAN control file
# Parameters:
# @param $1 Destination (install) folder
# @param $2 Package name
# @param $3 Version
# @param $4 Architecture
# @param $5 Dependences
# @param $6 Maintainer
# @param $7 Description
create_deb_control()
{
    # Create DEBIAN control file
    echo $ANSI_BROWN"Create DEBIAN control file:"$ANSI_NONE
    echo ""
    mkdir $1/DEBIAN
    cat << EOF > $1/DEBIAN/control
Package: $2
Version: $3
Section: libdevel
Priority: optional
Architecture: $4
Depends: $5
Maintainer: $6
Description: $7

EOF
}

# Build package
# Parameters:
# @param $1 Packet folder (or deb packet file name)
build_deb_package()
{
    echo $ANSI_BROWN"Build package:"$ANSI_NONE
    echo ""
    if [ -f "$1.deb" ]
    then
        rm $1.deb
    fi
    dpkg-deb --build $1
    rm -rf $1
    echo ""
}

# Install and run application to check created package
# Parameters:
# @param $1 Packet folder (or deb packet file name)
# @param $2 Application name to run with parameters
install_run_deb()
{
    echo $ANSI_BROWN"Install package and run application to check created package:"$ANSI_NONE
    echo ""
    set +e
    sudo dpkg -i $1.deb
    set -e
    sudo apt-get install -y -f
    echo ""
    echo $ANSI_BROWN"Run application:"$ANSI_NONE
    echo ""
    $2
    echo ""
}

# Show version of installed depends
show_teonet_depends()
{
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
}

# Remove package  
# Parameters:
# @param $1 Package name
apt_remove()
{
    echo $ANSI_BROWN"Remove package:"$ANSI_NONE
    echo ""
    sudo apt-get remove -y $1
    sudo apt-get autoremove -y
    echo ""
}

# Create DEB repository
# Parameters:
# @param $1 Repository folder
# @param $2 UBUNTU/DEBIAN sub-folder
# @param $3 Repository origin
# @param $4 Repository code name
# @param $5 Keys folder
# Set global variables:
# REPO_JUST_CREATED=1
create_deb_repo()
{
    # Create working directory
    if [ ! -d "$1" ];   then
        mkdir $1
    fi

    local REPO="$1/$2"

    # Create repository and configuration files
    if [ ! -d "$REPO" ]; then     
        echo $ANSI_BROWN"Create repository and configuration files:"$ANSI_NONE
        echo ""

        mkdir $REPO
        mkdir $REPO/conf 

        cat << EOF > $REPO/conf/distributions
Origin: $3
Label: $3
Suite: stable
Codename: $4
Version: 0.1
Architectures: i386 amd64 source
Components: main
Description: $3
SignWith: yes
EOF

        cat << EOF > $REPO/conf/options

verbose
ask-passphrase
basedir .

EOF

        # Import repository keys to this host
        #echo "Before Import repository keys..."
        str=`gpg --list-keys | grep "repository <repo@ksproject.org>"`
        if [ -z "$str" ]; then 
          echo $ANSI_BROWN"Add repository keys to this host:"$ANSI_NONE
          gpg --allow-secret-key-import --import $5/deb-sec.gpg.key
          gpg --import $5/deb.gpg.key
        fi
        #echo "After Import repository keys..."

        # Remove keys from this host
        # gpg --delete-secret-key  "repository <repo@ksproject.org>"
        # gpg --delete-key  "repository <repo@ksproject.org>"

        # Export key to repository folder
        mkdir $REPO/key
        gpg --armor --export repository repo@ksproject.org >> $REPO/key/deb.gpg.key
        # gpg --armor --export-secret-key repository repo@ksproject.org >> $REPO/key/deb-sec.gpg.key

        # Create the repository tree
        reprepro --ask-passphrase -Vb $REPO export

        REPO_JUST_CREATED=1
        echo ""
    fi
}

# Add DEB packages to local repository
# Parameters:
# @param $1 Repository folder with sub-folder
# @param $2 Repository code name
# @param $3 Package name
add_deb_package()
{
    echo $ANSI_BROWN"Add DEB package to local repository:"$ANSI_NONE
    echo ""
    reprepro --ask-passphrase -Vb $1 includedeb $2 $3.deb
    echo ""
}

#-------------------------------------------------------------------------------