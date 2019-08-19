FROM  ubuntu:18.04

WORKDIR /teonet

RUN apt-get update && \
    apt-get -y upgrade

# Autoconf dependencies
RUN apt-get -y install autoconf intltool libtool libglib2.0-dev doxygen make gcc

# Project dependencies
RUN apt-get install -y libssl-dev libev-dev libconfuse-dev uuid-dev libcunit1-dev cpputest libcurl4-openssl-dev

# copy teonet
COPY . /teonet

# Libtuntap
RUN apt-get install -y cmake g++ unzip
RUN cd /teonet/distr && \
    unzip libtuntap.zip && \
    cd libtuntap-master && \
    cmake ./ && \
    make && \
    make install

# Update system dynamic libraries configuration
RUN ldconfig

# build teonet
RUN cd /teonet && autoreconf --force --install
RUN ./autogen.sh --prefix=/usr/local
RUN make
RUN make install
RUN cp /teonet/src/.libs/libteonet* /usr/lib/
