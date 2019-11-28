#!/bin/bash
UDP_PORT=8125
docker run --rm --name=teonet-exporter \
    -p 9123:9102 \
    -p $UDP_PORT:$UDP_PORT/udp \
    -v $PWD/mapping.yml:/tmp/mapping.yml \
    prom/statsd-exporter \
        --log.level=debug \
        --statsd.mapping-config=/tmp/mapping.yml \
        --statsd.listen-udp=:$UDP_PORT \
        --web.listen-address=:9102
