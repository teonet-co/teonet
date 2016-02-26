FROM debian:jessie

RUN apt-get update && \
    apt-get install rsyslog --no-install-recommends -y && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY rsyslog.conf /etc/
VOLUME /var/run/rsyslog/dev

CMD ["rsyslogd", "-n"]

#
#
# Get from
# https://github.com/jpetazzo/syslogdocker/issues/4#issue-95955147
#
# Build
# docker build -t syslog .
#
# Run
# docker run --name syslog -d syslog
#
# Test execute in other docker
# docker run -it --volumes-from syslog ubuntu bash 
#   # ln -sf /var/run/rsyslog/dev/log /dev/log && logger "Hello world!"
#
