# Teoloadsi-js application 

Teonet simple load JS


## Dependences

### Install NodeJS

The is node based project so you should have node installed. Upgrade node to latest version:

    sudo apt-get install -y npm curl

    sudo npm cache clean -f
    sudo npm install -g n
    sudo n stable


### Install teonet library

### Ubuntu

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

#### Install library

    sudo apt-get install -y libteonet-dev


#### Check library installation

    teovpn -?


## Run this node application:  

    node app teo-node

## License

The MIT license

Copyright (c) 2018, Kirill Scherba


## *Developer Notes

### Teonet developer documentation:  

http://repo.ksproject.org/docs/teonet/


### Teonet events:  

http://repo.ksproject.org/docs/teonet/ev__mgr_8h.html#ad7b9bff24cb809ad64c305b3ec3a21fe


### Node FFI Tutorial:  

https://github.com/node-ffi/node-ffi/wiki/Node-FFI-Tutorial
