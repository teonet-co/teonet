#!/bin/sh
# 
# File:   make_rpm.sh
# Author: Kirill Scherba <kirill@scherba.ru>
#
# Created on Aug 28, 2015, 1:47:35 AM
#

VER=$1
if [ -z "$2" ]
  then
    ARCH="x86_64"
  else
    ARCH=$2
fi
PWD=`pwd`
REPO=../repo
VER_ARCH=$VER"_"$ARCH
RPMBUILD=~/rpmbuild
PREFIX=/usr
RELEASE=1

PACKET_NAME="libteonet"
PACKET_SUMMARY="Teonet library version $VER"

# 1. create your rpm build env for RPM < 4.6,4.7

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
#mkdir -p $PACKET_NAME-$VER/usr/bin
#mkdir -p $PACKET_NAME-$VER/etc/$PACKET_NAME
#install -m 755 $PACKET_NAME $PACKET_NAME-$VER/usr/bin
#install -m 644 $PACKET_NAME.conf $PACKET_NAME-$VER/etc/$PACKET_NAME/

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

tar -zcvf $PACKET_NAME-$VER.tar.gz $PACKET_NAME-$VER/
rm -rf $PACKET_NAME-$VER/

# 3. Copy to the sources dir

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


#4. build the source and the binary rpm

rpmbuild -ba $RPMBUILD/SPECS/$PACKET_NAME.spec
