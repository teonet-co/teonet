#!/bin/sh
#
# File:   make_deb_inc.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Bash functions to build sources, create DEB package add it to repository
#
# Created on Aug 31, 2015, 1:06:27 AM
#

# Include make deb functions
# PWD=`pwd`
. "$PWD/sh/make_inc.sh"

# Function ---------------------------------------------------------------------

# Check parameters and set defaults (specific for libteonet)
# Parameters:
# @param $1 Version (required)
# @param $2 Library HI version (default 0)
# @param $3 Library version (default 0.0.0)
# @param $4 Release (default 1)
# @param $5 Architecture (default and64)
# @param $6 Reserved
# @param $7 Package name (default libteonet-dev)
# @param $8 Package description (default ...)
# Set global variables:
# VER_ONLY=$1
# LIBRARY_HI_VERSION=$2
# LIBRARY_VERSION=$3
# RELEASE=$4
# ARCH=$5
# VER=$1-$RELEASE
# PACKET_NAME=$7
# PACKET_DESCRIPTION=$8
check_param()
{
    # The first parameter is required
    # $1
    if [ -z "$1" ]; then
        echo The first parameter is required
        exit 1
    fi
    VER_ONLY=$1

    # $2
    if [ -z "$2" ]; then
        LIBRARY_HI_VERSION=0
    else
        LIBRARY_HI_VERSION=$2
    fi

    # $3
    if [ -z "$3" ]; then
        LIBRARY_VERSION=0
    else
        LIBRARY_VERSION=$3
    fi

    # $4
    if [ -z "$4" ]; then
        RELEASE=1
      else
        RELEASE=$4
    fi

    # $5
    if [ -z "$5" ]; then
        ARCH="amd64"
      else
        ARCH=$5
    fi
    VER=$1-$RELEASE

    # $6
    # Reserved

    # $7
    if [ -z "$7" ]; then
        PACKET_NAME="libteonet-dev"
    else
        PACKET_NAME=$7
    fi

    # $8
    if [ -z "$8" ]; then
        PACKET_DESCRIPTION="Teonet library version $VER
     Mesh network library."
    else
        PACKET_DESCRIPTION=$8
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
    #local DEP
    if [ -z "$5" ]; then
        local DEP=""
    else
        local DEP="
Depends: $5"
    fi
    DEPENDS=
    cat << EOF > $1/DEBIAN/control
Package: $2
Version: $3
Section: libdevel
Priority: optional
Architecture: $4$DEP
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