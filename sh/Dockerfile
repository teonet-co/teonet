FROM  ubuntu:18.04 as builder

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
RUN apt-get install -y cmake g++ unzip sudo
RUN cd /teonet/distr && \
    unzip libtuntap.zip && \
    cd libtuntap-master && \
    cmake ./ && \
    make -j6 && \
    make install

# Update system dynamic libraries configuration
RUN ldconfig

# build teonet
RUN cd /teonet && autoreconf --force --install
RUN ./autogen.sh --prefix=/usr/local
RUN ./configure --prefix=/usr/local CXXFLAGS="-g -O0" CFLAGS="-g -O0"
RUN make -j6
#RUN make install
#RUN cp /teonet/src/.libs/libteonet* /usr/lib/

RUN sh/make_libtuntap.sh
RUN sh/make_package deb


FROM  ubuntu:18.04

RUN apt-get update && apt-get -y upgrade

WORKDIR /teonet
COPY --from=builder /teonet/*.deb /teonet/
RUN apt-get install -y libssl-dev libev-dev libconfuse-dev uuid-dev
RUN apt-get install -y /teonet/*.deb
