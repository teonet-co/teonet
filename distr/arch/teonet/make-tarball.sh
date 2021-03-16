#!/bin/sh


pkgver="0.4.6"

# 
# simple
#
# tar -czf teonet-0.4.6.tar.gz ../../../../teonet
#
# form github
#
git clone https://github.com/teonet-co/teonet teonet-$pkgver
cd teonet-$pkgver
git submodule update --init --recursive
#
cd ../
tar -czf teonet-$pkgver.tar.gz teonet-$pkgver
