# Teonet Dockerfile

#FROM       ubuntu:16.04
FROM       ubuntu:18.04
MAINTAINER Kirill Scherba <kirill@scherba.ru>

# Add teonet repository
RUN apt-get update && apt-get install -y gnupg2 git
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8CC88F3BE7D6113C
#RUN printf "\ndeb http://repo.ksproject.org/ubuntu/ teonet main\n" >> /etc/apt/sources.list
RUN printf "\ndeb http://$REPO_USER:$REPO_PASSWORD@repo2.ksproject.org/repo/ubuntu/ teonet main\n" >> /etc/apt/sources.list

# Install teonet
RUN apt-get update && apt-get install -y \
    libteonet-dev \
    && ldconfig
