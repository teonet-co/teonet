#!/bin/sh
# 
# File:   filebeat_inst.sh
# Author: kirill
#
# Created on Feb 25, 2016, 1:34:13 PM
#


# Install Filebeat:
sudo apt-get update
sudo apt-get install -y curl apt-transport-https
curl https://packages.elasticsearch.org/GPG-KEY-elasticsearch | sudo apt-key add -
echo "deb https://packages.elastic.co/beats/apt stable main" |  sudo tee -a /etc/apt/sources.list.d/beats.list
sudo apt-get update 
sudo apt-get install -y filebeat
sudo update-rc.d filebeat defaults 95 10
sudo update-rc.d rsyslog defaults 95 10

# Install default configuration file
sudo cat <<EOT > /etc/filebeat/filebeat.yml
output:
  logstash:
    enabled: true
    hosts:
      - elk:5044
    tls:
      certificate_authorities:
        - /etc/pki/tls/certs/logstash-beats.crt
    timeout: 15

filebeat:
  prospectors:
    -
      paths:
        - /var/log/syslog
#        - /var/log/auth.log
      document_type: syslog
#    -
#      paths:
#        - "/var/log/nginx/*.log"
#      document_type: nginx-access
EOT

# Install certificate
sudo mkdir /etc/pki/
sudo mkdir /etc/pki/tls/
sudo mkdir /etc/pki/tls/certs/
sudo cat <<EOT > /etc/pki/tls/certs/logstash-beats.crt
-----BEGIN CERTIFICATE-----
MIIC6zCCAdOgAwIBAgIJANPZwuf+5wTLMA0GCSqGSIb3DQEBCwUAMAwxCjAIBgNV
BAMMASowHhcNMTUxMjI4MTA0NTMyWhcNMjUxMjI1MTA0NTMyWjAMMQowCAYDVQQD
DAEqMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAp+jHFvhyYKiPXc7k
0c33f2QV+1hHNyW/uwcJbp5jG82cuQ41v70Z1+b2veBW4sUlDY3yAIEOPSUD8ASt
9m72CAo4xlwYKDvm/Sa3KJtDk0NrQiz6PPyBUFsY+Bj3xn6Nz1RW5YaP+Q1Hjnks
PEyQu4vLgfTSGYBHLD4gvs8wDWY7aaKf8DfuP7Ov74Qlj2GOxnmiDEF4tirlko0r
qQcvBgujCqA7rNoG+QDmkn3VrxtX8mKF72bxQ7USCyoxD4cWV2mU2HD2Maed3KHj
KAvDAzSyBMjI+qi9IlPN5MR7rVqUV0VlSKXBVPct6NG7x4WRwnoKjTXnr3CRADD0
4uvbQQIDAQABo1AwTjAdBgNVHQ4EFgQUVFurgDwdcgnCYxszc0dWMWhB3DswHwYD
VR0jBBgwFoAUVFurgDwdcgnCYxszc0dWMWhB3DswDAYDVR0TBAUwAwEB/zANBgkq
hkiG9w0BAQsFAAOCAQEAaLSytepMb5LXzOPr9OiuZjTk21a2C84k96f4uqGqKV/s
okTTKD0NdeY/IUIINMq4/ERiqn6YDgPgHIYvQheWqnJ8ir69ODcYCpsMXIPau1ow
T8c108BEHqBMEjkOQ5LrEjyvLa/29qJ5JsSSiULHvS917nVgY6xhcnRZ0AhuJkiI
ARKXwpO5tqJi6BtgzX/3VDSOgVZbvX1uX51Fe9gWwPDgipnYaE/t9TGzJEhKwSah
kNr+7RM+Glsv9rx1KcWcx4xxY3basG3/KwvsGAFPvk5tXbZ780VuNFTTZw7q3p8O
Gk1zQUBOie0naS0afype5qFMPp586SF/2xAeb68gLg==
-----END CERTIFICATE-----
EOT


# Startup script
sudo cat <<EOT > /root/teonet_log
#!/bin/sh
#
# teonet_log
#
# This script is executed at the end of each multiuser runlevel.
# Make sure that the script will "exit 0" on success or any other
# value on error.
#
# In order to enable or disable this script just change the execution
# bits.
#
# By default this script does nothing.

if [ ! -f /root/.teonet_log ]; then

    # Add elk to host
    sudo echo "172.17.0.1	elk" >> /etc/hosts

    # Load the default index template in Elasticsearch
    curl -XPUT 'http://elk:9200/_template/filebeat?pretty' -d@/etc/filebeat/filebeat.template.json

    # Start Filebeat:
    sudo /etc/init.d/filebeat start
    #sudo /etc/init.d/rsyslog start

    sudo echo "Installed ..." > /root/.teonet_log

fi

#exec "$@"

#sudo exec "$@"
#exit 0
EOT
#
sudo chmod +x /root/teonet_log
#
#sudo update-rc.d -f teonet_log remove
#sudo update-rc.d teonet_log defaults
#
#sudo update-rc.d -f rc.local remove
#sudo update-rc.d rc.local defaults

echo "Done"

# Add elk to host
#sudo cat  <<EOT >> /etc/hosts
#172.17.0.1  elk
#EOT

# Load the default index template in Elasticsearch
# curl -XPUT 'http://elk:9200/_template/filebeat?pretty' -d@/etc/filebeat/filebeat.template.json

# Start Filebeat:
# sudo /etc/init.d/filebeat start
# sudo /etc/init.d/rsyslog start


# Docker image create
# docker build -f Dockerfile_log .
# docker build -f Dockerfile_log -t gitlab.ksproject.org:5000/teonet/teonet_log .
# docker push gitlab.ksproject.org:5000/teonet/teonet_log

# Run teonet_log
# docker run -ti gitlab.ksproject.org:5000/teonet/teonet_log
