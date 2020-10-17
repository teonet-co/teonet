FROM ubuntu:latest
MAINTAINER Kirill Scherba <kirill@scherba.ru>

RUN apt-get update && apt-get install -y tinc

EXPOSE 657/tcp 657/udp
VOLUME /etc/tinc

ENTRYPOINT [ "/usr/sbin/tincd" ]
CMD [ "start", "-D", "-U", "nobody" ]

################################################################################
#
# Build command
# docker build gitlab.ksproject.org:5000/teonet/tinc .
#
#
# Run command
#
# Tinc:
# docker run -d --restart=always --name tinc --net=host --device=/dev/net/tun --cap-add NET_ADMIN --volume /etc/tinc:/etc/tinc gitlab.ksproject.org:5000/teonet/tinc -n kekalan-br -D
#
################################################################################
