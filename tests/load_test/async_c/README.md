# Teonet async test application

## 1. Description



## 2. Installation

### 2.1 Install project with submodules

    git clone No repository yet
    cd teoasync_c
    git submodule update --init

### 2.2 Install Dependences


### 2.2.1 Libteonet

DEB / RPM repository: http://repo.ksproject.org

#### 2.2.1.1 UBUNTU

http://repo.ksproject.org/ubuntu/

#### Add repository

    sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8CC88F3BE7D6113C
    sudo apt-get install -y software-properties-common
    sudo add-apt-repository "deb http://repo.ksproject.org/ubuntu/ teonet main"
    sudo apt-get update

#### Install

    sudo apt-get install -y libteonet-dev


#### 2.2.1.2 CENTOS / RHEL / FEDORA

http://repo.ksproject.org/rhel/x86_64/

#### Add repository

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

#### 2.2.1.3 SUSE

#### Add repository

    zypper ar -f http://repo.ksproject.org/opensuse/x86_64/ teonet

#### Install
    
    zypper in -y libteonet
    ldconfig

### 2.2.2 OpenSSL

    \todo Add description

## 3. Generate your application sources (first time when got sources from repository)

    ./autogen.sh


## 4. Make your application 

    make

## 4.1 Using autoscan to Create configure.ac

After make some global changes in sources code use ```autoscan``` to update projects 
configure.ac

## 5. Run 
    
    src/teoasync_c teoasync_c

## 6. Teonet documentation

See teonet documentation at: http://repo.ksproject.org/docs/teonet/
