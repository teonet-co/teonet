#!/bin/sh

# system dependencies
sudo pacman -Sy --noconfirm base-devel cmake
sudo pacman -S --noconfirm intltool doxygen
sudo pacman -S --noconfirm libev confuse cunit

# cpputest
cd distr/arch/cpputest
makepkg -si --noconfirm
rm -rf pkg src cpputest-3.8*
cd ../../..

# tuntap
#
# package
cd distr/arch/libtuntap
cp ../../libtuntap.zip ./
makepkg -si --noconfirm
rm -rf pkg src libtuntap*
cd ../../..
#
# local source code
cd libs/libtuntap
patch < ../../distr/arch/libtuntap/CMakeLists.txt-2.patch
cmake ./
cd ../..

#export TEONET_HOME=`pwd`

