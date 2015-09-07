# Teonet Dockerfile

FROM       ubuntu
MAINTAINER Kirill Scherba <kirill@scherba.ru>

# Add teonet repository
RUN sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8CC88F3BE7D6113C
RUN sudo printf "\ndeb http://repo.ksproject.org/ubuntu/ teonet main\n" >> /etc/apt/sources.list
RUN sudo apt-get update

# Install teonet
RUN sudo apt-get install -y libteonet-dev
RUN sudo ldconfig
