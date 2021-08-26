# Build Teonet library from sources

<p align="center">
<a href="https://www.codacy.com/gh/teonet-co/teonet/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=teonet-co/teonet&amp;utm_campaign=Badge_Grade"><img src="https://app.codacy.com/project/badge/Grade/e33f91daebdb4df39dd908402dbf37d9"/></a>
<a href="https://www.codefactor.io/repository/github/teonet-co/teonet"><img src="https://www.codefactor.io/repository/github/teonet-co/teonet/badge" alt="CodeFactor" /></a>
<a href="https://github.com/teonet-co/teonet/blob/develop/COPYING"><img src="https://img.shields.io/badge/license-GPLv3-green" alt="License" /></a>
</p>

<p align="center">
<a href="https://circleci.com/gh/teonet-co/workflows/teonet"><img src="https://img.shields.io/circleci/build/github/teonet-co/teonet.svg?label=circleci" alt="CircleCI Status" /></a>
<a href="https://teonet-co.github.io/teonet/"><img src="https://codedocs.xyz/teonet-co/teonet.svg" alt="Docs Status" /></a>
<a href="https://github.com/teonet-co/teonet/releases/latest"><img src="https://img.shields.io/github/release/teonet-co/teonet.svg?maxAge=600" alt="Latest release" /></a>
</p>
<p align="center">
<a href="https://github.com/teonet-co/teonet/issues"><img alt="GitHub issues" src="https://img.shields.io/github/issues/teonet-co/teonet"></a>
<a href="https://github.com/teonet-co/teonet/graphs/contributors"><img alt="GitHub contributors" src="https://img.shields.io/github/contributors/teonet-co/teonet"></a>
</p>

## Clone git repository  to get code

    git clone git@gitlab.ksproject.org:teonet/teonet.git
    cd teonet
    git submodule update --init --recursive

## Update submodules to latest version

    git submodule update --remote --merge

* at error: "SSL certificate problem: unable to get local issuer 
certificate while accessing https ..." execute git command:

    git config --global http.sslVerify false

## First time, after got sources from subversion repository

### Ubuntu
    sh/build-ubuntu.sh
    ./autogen.sh
    ./configure
    make

#### Create debian packages and install it

It is possible use deb packages to install teonet instead of `make install` command. To create teonet debian packages use next command:

    make deb-package

To install teonet debian package use next commands (there X.X.X is current teonet version):

    sudo dpkg -i libtuntap-dev_0.3.0_amd64.deb
    sudo dpkg -i libteonet-dev_X.X.X-1_amd64.deb

 To remove installed debian packages use next commands:

    sudo dpkg -r libteonet-dev
    sudo dpkg -r libtuntap-dev

### Manjaro
    autoreconf --force --install
    ./autogen.sh
    ./configure
    make

## At errors try running

    autoreconf --force --install
    ./configure
    make

## Third party notes

libev - A full-featured and high-performance event loop

    URL: http://libev.schmorp.de/

PBL - The Program Base Library

    URL: http://www.mission-base.com/peter/source/

MYSQL - MariaDB or MySQL connector (used in teoweb application)

    sudo apt-get install libmysqlclient-dev

CUnit - A Unit Testing Framework for C

    sudo apt-get install libcunit1-dev

Cpputest - CppUTest unit testing and mocking framework for C/C++

    sudo apt-get install cpputest

## Install from repositories notes

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

    # Build teonet
    cd /$HOME/Projects/teonet
    make clean && make

    # Copy teonet library to bin folder for use with node applications
    sh/libteonet_copy.sh

    # L0 and websocket server
    cd /$HOME/Projects/teohws
    make clean && make
    src/teohws teo-hws -p 9009 --l0_allow --l0_tcp_port 9009

    # L0 server web config file
    #
    # document_root = "/var/www"
    # http_port = 8082
    # l0_server_name = "127.0.0.1"
    # l0_server_port = 9009
    # auth_server_url = "http://teomac.ksproject.org:1234/api/auth/"

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

# Sume LXC notes

When you run teonet in LXC container you posible need to open(forward) UDP port (8000 in exampe below) in LXC host:

    iptables -t nat -A PREROUTING -i eth0 -p udp --dport 8000 -j DNAT --to 10.29.213.63:8000
