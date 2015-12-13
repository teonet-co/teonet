/** 
 * File:   teo_auth.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Module to connect to Authentication server AuthPrototype
 *
 * Created on December 1, 2015, 1:51 PM
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include "teo_auth.h"
#include "conf.h"


static int teoAuthProccess(teoAuthClass *ta, const char *method, 
        const char *url, const char *data, const char *header, 
        void *nc_p, command_callback callback);

static int send_request(char *url, void *data, size_t data_len, char *header, 
        void *nc_p, command_callback callback);

/**
 * Authentication module thread function
 * 
 * @param kh Pointer to ksnetEvMgrClass
 * 
 * @return 
 */
static void* auth_thread(void *ta_p) {

    printf("Starting authentication thread ...\n");
    
    teoAuthClass *ta = ta_p;
    ta->stopped = 0;
    ta->stop = 0;
    
    pthread_mutex_lock(&ta->cv_mutex);
    for (;;) {
        
        printf("Authentication thread wait signal\n");
        ta->running = 0;
        pthread_cond_wait(&ta->cv_threshold, &ta->cv_mutex); 
        ta->running = 1;
        printf("Authentication thread got signal\n");            
        if(ta->stop) break;
        
        // Get first element from list and process it
        while(!pblListIsEmpty(ta->list)) {
            teoAuthData *element = pblListRemove(ta->list);
            if(element != (void*)-1 && element != NULL) {
                teoAuthProccess(ta, element->method, element->url, 
                        element->data,  element->headers, 
                        element->nc_p, element->callback);
            }
        }
    }
    pthread_mutex_unlock(&ta->cv_mutex);
    
    printf("Authentication thread stopped\n");
    
    ta->stopped = 1;
    ta->running = 0;
    
    return NULL;
}

/**
 * Send signal to authentication thread
 * 
 * @param ta
 */
static void auth_thread_signal(teoAuthClass *ta) {
    
    // Send signal to Authentication thread
    if(!ta->stopped && !ta->running) {
        
        pthread_mutex_lock(&ta->cv_mutex);
        printf("Send signal to Authentication thread\n");
        pthread_cond_signal(&ta->cv_threshold);
        pthread_mutex_unlock(&ta->cv_mutex);
    }
}

/**
 * Initialize Teonet authenticate module
 * 
 * @return 
 */
teoAuthClass *teoAuthInit(ksnHTTPClass *kh) {
    
    teoAuthClass *ta = malloc(sizeof(teoAuthClass));
    ta->list = pblListNewArrayList();
    ta->kh = kh;
    
    // In windows, this will initialize the winsock stuff 
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    // Check for errors 
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed: %s\n", 
                curl_easy_strerror(res));
        //return 1;
    }
    
    // Initialize mutex
    pthread_cond_init(&ta->cv_threshold, NULL);
    pthread_mutex_init(&ta->async_mutex, NULL);
    pthread_mutex_init(&ta->cv_mutex, NULL);
     
    // Start authentication module thread
    int err = pthread_create(&ta->tid, NULL, auth_thread, (void*)ta);
    if (err != 0) printf("Can't create Authentication thread :[%s]\n", strerror(err));
    else printf("Authentication thread created successfully\n");
    
    return ta;
}

/**
 * Store authentication command in list
 * 
 * @param ta Pointer to teoAuthClass
 * @param method
 * @param url
 * @param data
 * @param headers
 * @param nc_p
 * @param callback
 * 
 * @return 
 */
int teoAuthProcessCommand(teoAuthClass *ta, const char *method, const char *url, 
        const char *data, const char *headers, void *nc_p, 
        command_callback callback) {
    
    // Create element and store it in list
    teoAuthData *element = malloc(sizeof(teoAuthData));
    //
    element->method = strdup(method);
    element->url = strdup(url);
    element->data = strdup(data);
    element->headers = strdup(headers);
    element->callback = callback;
    element->nc_p = nc_p;
    //
    pthread_mutex_lock (&ta->async_mutex);
    pblListAdd(ta->list, element);
    pthread_mutex_unlock (&ta->async_mutex);
    
    // Send signal to Authentication thread
    auth_thread_signal(ta);

    return 1;
}

/**
 * Free teonet authentication data element
 * 
 * @param element
 * @return 
 */
static void teoAuthDataFree(teoAuthData *element) {
    
    free(element->method);
    free(element->url);
    free(element->data);
    free(element->headers);
}

/**
 * Process authentication command
 * 
 * @param ta Pointer to teoAuthClass
 * @param method
 * @param url
 * @param data
 * @param headers
 * @return 
 */
static int teoAuthProccess(teoAuthClass *ta, const char *method, 
        const char *url, const char *data, const char *header, 
        void *nc_p, command_callback callback) {
    
    int valid_url = 0;
    
    // Validate URL
    if(strcmp(url, "register-client")) {
        valid_url = 1;
    }
    
    else if(strcmp(url, "register")) {
        valid_url = 1;
    }
    
    else if(strcmp(url, "login")) {
        valid_url = 1;
    }
    
    else if(strcmp(url, "refresh")) {
        valid_url = 1;
    }
    
    else if(strcmp(url, "restore")) {
        valid_url = 1;
        
    }
    
    else if(strcmp(url, "change-password")) {
        valid_url = 1;
        
    }
        
    else if(strcmp(url, "me")) { // Method GET and without Data
        valid_url = 1;
    }
    
        
    if(valid_url) {
    
        char full_url[KSN_BUFFER_SM_SIZE];
        snprintf(full_url, KSN_BUFFER_SM_SIZE,
                "%s%s", ta->kh->conf->auth_server_url, url);
        
        // Send authentication request        
        send_request(full_url, (void*) data, strlen(data), (char*)header, nc_p, 
                callback);
    }
    
    return 0;
}

/**
 * Destroy Teonet authenticate module
 * 
 * @param ta
 */
void teoAuthDestroy(teoAuthClass *ta) {
    
    if(ta != NULL) {
        
        // Stop Authentication thread
        ta->stop = 1;
        auth_thread_signal(ta);
        while(!ta->stopped) usleep(10000); // waiting thread stop per 10 ms
        
        pthread_cond_destroy(&ta->cv_threshold);
        pthread_mutex_destroy(&ta->async_mutex);
        pthread_mutex_destroy(&ta->cv_mutex);

        // Loop list and destroy all loaded modules
        PblIterator *it =  pblListIterator(ta->list);
        if(it != NULL) {     
            
            while(pblIteratorHasNext(it)) {        

              teoAuthDataFree(pblIteratorNext(it));
              
            }
            pblIteratorFree(it);
        }

        // Free list
        pblListFree(ta->list);

        // Free module class
        free(ta);
        
        // Cleanup curl
        curl_global_cleanup();
    } 
}

// Send POST/GET Requests to HTTP Server (with curl) --------------------------
//
//
//
 
struct WriteThis {
    
  const char *readptr;
  long sizeleft;
};

/**
 * C string implementation data structure
 */
struct string {
    
  char *ptr;
  size_t len;
  
};
 
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp) {

  struct WriteThis *pooh = (struct WriteThis *)userp;
  //struct string *pooh = (struct string *)userp;
 
  if(size*nmemb < 1)
    return 0;
 
  if(pooh->sizeleft) {
    *(char *)ptr = pooh->readptr[0]; /* copy one single byte */ 
    pooh->readptr++;                 /* advance pointer */ 
    pooh->sizeleft--;                /* less data left */ 
    return 1;                        /* we return 1 byte at a time! */ 
  }
 
  return 0;                          /* no more data left to deliver */ 
}

/**
 * Initialize (create) C string
 * 
 * @param s
 */
static void init_string(struct string *s) {
    
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

/**
 * Curl write callback
 * 
 * @param ptr
 * @param size
 * @param nmemb
 * @param s
 * @return 
 */
static size_t write_callback(void *ptr, size_t size, size_t nmemb, struct string *s) {

  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}
 
/**
 * Send request and get result
 * 
 * @param url
 * @param data
 * @param data_len
 * @return 
 */
static int send_request(char *url, void *data, size_t data_len, char *header,
        void *nc_p, command_callback callback) {

  CURL *curl;
  CURLcode res;
 
  struct WriteThis pooh;
  pooh.readptr = data;
  pooh.sizeleft = data_len;
  
  printf("### Post data: %s\n", (char*)data);
  
  struct string s;
  init_string(&s);
  
  // get a curl handle
  curl = curl_easy_init();
  if(curl) {
      
    /* First set the URL that is about to receive our POST. */ 
    curl_easy_setopt(curl, CURLOPT_URL, url); 
 
    /* Now specify we want to POST data */ 
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    
    struct curl_slist *headers = curl_slist_append(NULL, header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
 
    /* we want to use our own read function */ 
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
 
    /* pointer to pass to our read function */ 
    curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
 
    /* get verbose debug output please */ 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
 
    /*
      If you use POST to a HTTP 1.1 server, you can send data without knowing
      the size before starting the POST if you use chunked encoding. You
      enable this by adding a header like "Transfer-Encoding: chunked" with
      CURLOPT_HTTPHEADER. With HTTP 1.0 or without chunked transfer, you must
      specify the size in the request.
    */ 
    #ifdef USE_CHUNKED
    {
      struct curl_slist *chunk = NULL;
 
      chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      /* use curl_slist_free_all() after the *perform() call to free this
         list again */ 
    }
    #else
    /* Set the expected POST size. If you want to POST large amounts of data,
       consider CURLOPT_POSTFIELDSIZE_LARGE */ 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, pooh.sizeleft);
    #endif
 
    #ifdef DISABLE_EXPECT
    /*
      Using POST with HTTP 1.1 implies the use of a "Expect: 100-continue"
      header.  You can disable this header with CURLOPT_HTTPHEADER as usual.
      NOTE: if you want chunked transfer too, you need to combine these two
      since you can only set one list of headers with CURLOPT_HTTPHEADER. */ 
 
    /* A less good option would be to enforce HTTP 1.0, but that might also
       have other implications. */ 
    {
      struct curl_slist *chunk = NULL;
 
      chunk = curl_slist_append(chunk, "Expect:");
      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      /* use curl_slist_free_all() after the *perform() call to free this
         list again */ 
    }
    #endif
 
    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    
    // Check for curl errors
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    
    // Success curl result
    else {
        
        // Get response info
        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        printf("Post response: HTTP CODE: %d\n%s\n", (int)http_code, s.ptr);
          
        // Create response json string
        size_t http_code_s_len = 32;
        char http_code_s[http_code_s_len];
        char *buf_format = "{ \"status\": %d, \"data\": %s%s%s }";
        size_t buf_len = strlen(buf_format) + http_code_s_len + strlen(s.ptr);
        char buf[buf_len];
        char *q = http_code != 200 ? "\"":"";
        snprintf(http_code_s, http_code_s_len, "%d", (int)http_code);
        snprintf(buf, buf_len, buf_format, (int)http_code, q, s.ptr, q);
        // Call response callback and send response
        callback(nc_p, http_code != 200 ? http_code_s : NULL, buf);
        // Free HTTP server response string
        free(s.ptr);
    }    
    
    // Always cleanup 
    curl_easy_cleanup(curl);
    
    // Free the custom headers
    curl_slist_free_all(headers);
  }
  
  return 0;
}
