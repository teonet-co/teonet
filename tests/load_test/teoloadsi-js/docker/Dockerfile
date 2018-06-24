# Teonet teoloadsi-js Dockerfile

FROM node
MAINTAINER Kirill Scherba <kirill@scherba.ru>

# Add Teonet repository and Install Teonet
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 8CC88F3BE7D6113C && \
    printf "\ndeb http://repo.ksproject.org/ubuntu/ teonet main\n" >> /etc/apt/sources.list && \
    apt-get update && apt-get upgrade -y && apt-get install -y libteonet-dev && ldconfig

# Copy project
ADD . /root/Projects/teoloadsi-js

# Set work folder and Install npm dependencies
WORKDIR /root/Projects/teoloadsi-js
RUN npm i

# Default command
CMD ["node",".","teo-load-si-js"]

# Build
# cd $HOME/Projects/teoloadsi-js
# docker build -t teoloadsi-js -f docker/Dockerfile .
#
# Run watcher as an example
# docker run -it --rm --name teoloadsi-js  teoloadsi-js node . teo-load-si-js
# docker run -it --rm --name teoloadsi-js -v $HOME/.teoloadsi-js:/root/.node teoloadsi-js node . teo-load-si-js
