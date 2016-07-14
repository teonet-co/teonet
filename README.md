# Build Teonet library from sources

## Clone git repository  to get code

    git clone git@gitlab.ksproject.org:teonet/teonet.git
    cd teonet
    git submodule update --init --recursive

## Update submodules

    git submodule update --remote --merge

* at error: "SSL certificate problem: unable to get local issuer 
certificate while accessing https ..." execute git command:

    git config --global http.sslVerify false

## First time, after got sources from subversion repository

    sh/build-ubuntu.sh
    ./autogen.sh
    make
 

## At errors try running:

    autoreconf --force --install
    ./configure
    make


## Third party notes:

libev - A full-featured and high-performance event loop

    URL: http://libev.schmorp.de/


PBL - The Program Base Library

    URL: http://www.mission-base.com/peter/source/

MYSQL - MariaDB or MySQL connector (used in teoweb application)

    sudo apt-get install libmysqlclient-dev


## Install from repositories notes:

    DEB / RPM repository: http://repo.ksproject.org

### UBUNTU

    http://repo.ksproject.org/ubuntu/

#### Add repository

Add repository key:  

    sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8CC88F3BE7D6113C
    
Add repository:    

    sudo apt-get install -y software-properties-common
    sudo add-apt-repository "deb http://repo.ksproject.org/ubuntu/ teonet main"
    
or add the following line to your /etc/apt/sources.list:  

    deb http://repo.ksproject.org/ubuntu/ teonet main
    
Update repositories database:    
    
    sudo apt-get update

#### Install

    sudo apt-get install -y libteonet-dev

#### Run

    teovpn -?


### CENTOS / RHEL / FEDORA

    http://repo.ksproject.org/rhel/x86_64/

# Add repository

    vi /etc/yum.repos.d/teonet.repo

    [teonet]
    name=Teonet library for RHEL / CentOS / Fedora
    baseurl=http://repo.ksproject.org/rhel/x86_64/
    enabled=1
    gpgcheck=0

#### Refresh

    # yum clean all

#### Install

    yum install libteonet
    ldconfig 

#### Run

    teovpn -?


### SUSE

#### Add repository

    zypper ar -f http://repo.ksproject.org/opensuse/x86_64/ teonet

#### Install
    
    zypper in -y libteonet
    ldconfig

#### Run

    teovpn -?
    

# Teonet local net configuration (run from sources folder)

```
# Build teonet
cd /$HOME/Projects/teonet
make clean && make

# Copy teonet library to bin folder for use with node applications
sh/libteonet_copy.sh

# L0 and websocket server
cd /$HOME/Projects/teohws
make clean && make
src/teohws teo-hws -p 9009 --l0_allow --l0_tcp_port 9009

# Auth helper server
cd /$HOME/Projects/teonode
node app/authasst/index.js teo-auth -a 127.0.0.1 -P 9009

# Teonet room controller
cd /$HOME/Projects/teoroom
make clean && make
src/teoroom teo-room -a 127.0.0.1 -P 9009

# Teonet match making controller
cd /$HOME/Projects/teomm
make clean && make
src/teomm teo-mm -a 127.0.0.1 -P 9009

# Teonet db
cd /$HOME/Projects/teodb
make clean && make
src/teodb teo-db -a 127.0.0.1 -P 9009
```
