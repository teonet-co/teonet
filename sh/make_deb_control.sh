#! /bin/sh
# Generate DEBIAN control file text

cat << EOF
Package: libteonet
Version: 0.0.7
Section: base
Priority: optional
Architecture: amd64
Depends: 
Maintainer: Kirill Scherba <kirill@scherba.ru>
Description: Teonet library
 When you need some sunshine, just run this
 small program!

EOF