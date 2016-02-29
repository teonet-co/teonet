# Teonet logging Dockerfile

#FROM gitlab.ksproject.org:5000/teonet/teonet
FROM ubuntu:14.04
MAINTAINER Kirill Scherba <kirill@scherba.ru>

# Add filebeat install script
ADD filebeat_install.sh ~/

# Install filebeat
WORKDIR ~/
RUN ./filebeat_install.sh
RUN rm ./filebeat_install.sh

CMD ["/root/teonet_log_run"]

#VOLUME /dev
#VOLUME /var/log

#
# Build
# docker build -t gitlab.ksproject.org:5000/teonet/filebeat .
#
# Run
# docker run --volumes-from syslog --name filebeat -d gitlab.ksproject.org:5000/teonet/filebeat
#
