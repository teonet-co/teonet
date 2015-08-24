#!/bin/sh
# Generate DEBIAN control file text

VER=$1

cat << EOF
Package: libteonet
Version: $VER
Section: base
Priority: optional
Architecture: amd64
Depends: 
Maintainer: Kirill Scherba <kirill@scherba.ru>
Description: Teonet library
 Mesh network library.

EOF