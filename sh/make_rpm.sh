#!/bin/sh
# 
# File:   make_rpm.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Created on Aug 28, 2015, 1:47:35 AM
#

# Parameters:
#
# @param $1 Version
# @param $2 Release 
# @param $3 Architecture
# @param $4 RPM subtype rpm yum zyp

set -e # exit at error

# The first parameter is required
if [ -z "$1" ]
  then
    exit -1
fi

VER=$1
if [ -z "$2" ]
  then
    RELEASE=1
  else
    RELEASE=$2
fi
if [ -z "$3" ]
  then
    ARCH="x86_64"
  else
    ARCH=$3
fi
if [ -z "$4" ]
  then
    RPM_SUBTYPE="rpm"
    INST="sudo apt-get install -y "
    RPM_DEV="rpm"
  else
    RPM_SUBTYPE=$4
    INST="sudo apt-get install -y "
    RPM_DEV="rpm"
    if [ "$RPM_SUBTYPE" = "yum" ]; then
        INST="yum install -y "
        RPM_DEV="rpm-build"
    fi
    if [ "$RPM_SUBTYPE" = "zyp" ]; then
        INST="zypper install -y "
        RPM_DEV="rpm-devel"
    fi
fi

#echo "Show params: \n1=$1\n2=$2\n3=$3\n4=$4\n"
#echo "RPM_SUBTYPE="$RPM_SUBTYPE
#echo "INST="$INST
#echo "RPM_DEV="$RPM_DEV
#echo ""
#
#exit


PWD=`pwd`
REPO=../repo
VER_ARCH=$VER"_"$ARCH
RPMBUILD=~/rpmbuild
PREFIX=/usr

PACKET_NAME="libteonet"
PACKET_SUMMARY="Teonet library version $VER"

ANSI_BROWN="\033[22;33m"
ANSI_NONE="\033[0m"

echo $ANSI_BROWN"Create RPM packet $PACKET_NAME""_$VER_ARCH"$ANSI_NONE
echo ""

#Update and upgrade build host
#echo $ANSI_BROWN"Update and upgrade build host:"$ANSI_NONE
#echo ""
#sudo apt-get update
#sudo apt-get -y upgrade
#echo ""

# 1. create your RPM build environment for RPM

if [ -d "$RPMBUILD" ]; then
    rm -fr $RPMBUILD
fi
mkdir $RPMBUILD #/{RPMS,SRPMS,BUILD,SOURCES,SPECS,tmp}
mkdir $RPMBUILD/RPMS
mkdir $RPMBUILD/SRPMS
mkdir $RPMBUILD/BUILD
mkdir $RPMBUILD/SOURCES
mkdir $RPMBUILD/SPECS
mkdir $RPMBUILD/tmp

cat <<EOF >~/.rpmmacros
%_topdir   %(echo $HOME)/rpmbuild
%_tmppath  %{_topdir}/tmp
EOF

# 2. create the tarball of your project

mkdir $PWD/$PACKET_NAME-$VER

# Configure and make
echo $ANSI_BROWN"Configure or autogen:"$ANSI_NONE
echo ""
if [ -f "autogen.sh" ]
then
./autogen.sh --prefix=$PREFIX
else
./configure --prefix=$PREFIX
fi
echo ""
echo $ANSI_BROWN"Make:"$ANSI_NONE
echo ""
make
echo ""

# Install to temporary folder 
echo $ANSI_BROWN"Make install to temporary folder:"$ANSI_NONE
echo ""
make install DESTDIR=$PWD/$PACKET_NAME"-"$VER
echo ""

# Create binary tarball
echo $ANSI_BROWN"Create binary tarball:"$ANSI_NONE
echo ""
tar -zcvf $PACKET_NAME-$VER.tar.gz $PACKET_NAME-$VER/
rm -rf $PACKET_NAME-$VER/
echo ""

# 3. Copy to the sources folder
echo $ANSI_BROWN"Copy files to the rpmbuild sources folder:"$ANSI_NONE
echo ""
mv -f $PACKET_NAME-$VER.tar.gz $RPMBUILD/SOURCES

cat <<EOF > $RPMBUILD/SPECS/$PACKET_NAME.spec
# Don't try fancy stuff like debuginfo, which is useless on binary-only
# packages. Don't strip binary too
# Be sure buildpolicy set to do nothing
%define        __spec_install_post %{nil}
%define          debug_package %{nil}
%define        __os_install_post %{_dbpath}/brp-compress

Summary: $PACKET_SUMMARY
Name: $PACKET_NAME
Version: $VER
Release: $RELEASE
License: GPL+
Group: Development/Tools
SOURCE0 : %{name}-%{version}.tar.gz
URL: http://$PACKET_NAME.company.com/

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
%{summary}

%prep
%setup -q

%build
# Empty section.

%install
rm -rf %{buildroot}
mkdir -p  %{buildroot}

# in builddir
cp -a * %{buildroot}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
$PREFIX/*
#%config(noreplace) %{_sysconfdir}/%{name}/%{name}.conf

%changelog
#* Thu Apr 24 2009  Elia Pinto <devzero2000@rpm5.org> 1.0-1
#- First Build

EOF
echo "done"
echo ""


# 4. build the source and the binary RPM
echo $ANSI_BROWN"Build the source and the binary RPM:"$ANSI_NONE
echo ""
$INST$RPM_DEV
rpmbuild -ba $RPMBUILD/SPECS/$PACKET_NAME.spec
echo ""

# 5. Add DEB packages to local repository
echo $ANSI_BROWN"Add REP package to local repository:"$ANSI_NONE
echo ""
if [ ! -d "$REPO/rhel/" ]; then
mkdir $REPO/rhel/
fi
if [ ! -d "$REPO/rhel/$ARCH/" ]; then
mkdir $REPO/rhel/$ARCH/
fi
cp $RPMBUILD/RPMS/$ARCH/* $REPO/rhel/$ARCH/
cp -r $RPMBUILD/SRPMS/ $REPO/rhel/
$INST"createrepo"
createrepo $REPO/rhel/$ARCH/

# Upload repository to remote host and Test Install and run application
if [ ! -z "$CI_BUILD_REF" ]; then
    
    # Upload repository to remote host
    # by ftp: 
    sh/make_deb_remote_upload.sh $ARCH

    # Install packet from remote repository
    # sh/make_rpm_remote_install.sh

fi
