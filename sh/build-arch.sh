#!/bin/sh

# system dependencies
sudo pacman -Sy --noconfirm base-devel
sudo pacman -S --noconfirm intltool doxygen
sudo pacman -S --noconfirm libev confuse 

# cpputest
cd distr/arch/cpputest
makepkg -si --noconfirm
rm -rf pkg src cpputest-3.8*
cd ../../..

# tuntap
cd distr/arch/libtuntap
cp ../../libtuntap.zip ./
makepkg -si --noconfirm
rm -rf pkg src libtuntap*
cd ../../..

#export TEONET_HOME=`pwd`

