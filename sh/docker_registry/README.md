# Docker registry

Docker registry at: gitlab.ksproject.org:5000  
  
The SSL certificate made it startssl: https://www.startssl.com/  
  
Folder files description:

Kirill_Scherba -- browser certificate to login to startssl
README.md -- this file
ca.pem -- startssl CA certificate
combine-crt.sh -- script to combine startssl certificates to ssl-combined.crt
create-registry.sh -- script to create docker registry
ssl-combined.crt -- combined startssl certificate
ssl-decrypted.key -- decrypted startssl domain key
ssl.crt -- domain startssl certificate
ssl.key -- domain startssl key
ssl.key.passwd -- domain startssl key password
sub.class1.server.ca.pem -- intermediate startssl certificate
  
To create the docker registry copy this folder to docker host (the docker should 
be set before) and start the ./create-registry.sh script
