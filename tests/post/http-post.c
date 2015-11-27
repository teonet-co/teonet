/* 
 * File:   http-post.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on November 26, 2015, 8:24 PM
 */

/**
 * Make it with: gcc -o http-post http-post.c -lcurl
 * 
 */

#include <stdio.h>
#include <curl/curl.h>
 
int main(void) {

  CURL *curl;
  CURLcode res;
 
  // In windows, this will init the winsock stuff 
  curl_global_init(CURL_GLOBAL_ALL);
 
  // Get a curl handle 
  curl = curl_easy_init();
  if(curl) {
      
    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. */ 
    curl_easy_setopt(curl, CURLOPT_URL, "http://172.17.0.3:1234/api/auth/register-client");
    
    // Now specify the POST data 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "type=http-post&project=teonet");

    // Perform the request, res will get the return code 
    res = curl_easy_perform(curl);
    
    // Check for errors 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    // Always cleanup 
    curl_easy_cleanup(curl);
  }
  
  curl_global_cleanup();
  return 0;
}