# Teonet repository HTTPS certificate  
  
Folder content:  
  
```  
README.md -- this file
ca.pem -- startssl CA file
combine-crt.sh -- script to combine certificates
ssl-decrypted.key -- decrypted key
ssl.crt -- repo.ksproject.org certificate
ssl.key -- certificate key
sub.class1.server.ca.pem -- intermediate startssl certificate
```  
   
Create ssl-combined.crt: ```$ combine-crt.sh```  
   
Copy ```ssl-combined.crt``` to ```SSLCertificateFile /var/www/httpd-cert/repo.ksproject.org.crt``` at the ```repo.ksproject.org``` host  
Copy ```ssl-decrypted.key``` to ```SSLCertificateKeyFile /var/www/httpd-cert/repo.ksproject.org.key``` at the ```repo.ksproject.org``` host  
