docker run -d -p 5000:5000 --restart=always --name registry \
-v `pwd`:/certs \
-e REGISTRY_HTTP_TLS_CERTIFICATE=/certs/ssl-combined.crt \
-e REGISTRY_HTTP_TLS_KEY=/certs/ssl-decrypted.key \
registry:2
