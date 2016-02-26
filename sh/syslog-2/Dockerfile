FROM ubuntu:14.04
RUN apt-get update -q
RUN apt-get install rsyslog
CMD rsyslogd -n
VOLUME /dev
VOLUME /var/log


#
#
# Get from
# https://github.com/jpetazzo/syslogdocker/blob/master/Dockerfile
#
# Build
# docker build -t syslog .
#
# Run
# docker run --name syslog -d syslog
#
# Test execute in other docker
# docker run -it --rm --volumes-from syslog ubuntu bash 
    # logger "Hello world!"
    # tail -f /var/log/syslog
#
#  docker run --volumes-from syslog ubuntu tail -f /var/log/syslog
#


