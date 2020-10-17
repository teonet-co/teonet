sudo pacman -Sy --noconfirm base-devel
sudo pacman -S --noconfirm intltool doxygen
sudo pacman -S --noconfirm libev confuse 
# cpputest
cd embedded
wget https://github.com/cpputest/cpputest/releases/download/v3.8/cpputest-3.8.tar.gz
tar xvzf cpputest-3.8.tar.gz
cd cpputest-3.8/
autoreconf -i
./configure
make
sudo make install
cd ..
rm -rf cpputest-3.8*
cd ../
# tuntap
sudo pacman -S --noconfirm cmake unzip
cd distr
unzip libtuntap.zip
cd libtuntap-master
cmake ./
make
sudo make install
cd ..
rm -fr libtuntap-master

#export TEONET_HOME=`pwd`

