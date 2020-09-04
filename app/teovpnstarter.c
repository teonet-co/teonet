#include <curl/curl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static const char* home_env_var = "HOME";
static const char* public_ipv4_var = "PUBLIC_IPV4";
static const char* public_ipv6_var = "PUBLIC_IPV6";

typedef struct AppSettings
{
    char* public_ipv4;
} AppSettings;

size_t save_public_ip_cb(void *ptr, size_t size, size_t nmemb, void *stream){
    AppSettings* settings = (AppSettings*)stream;
    size_t buffer_size = strlen(public_ipv4_var) + 1 + nmemb + 1;//PUBLIC_IPV4=111.111.111.111 + terminating 0
    settings->public_ipv4 = (char*)malloc(buffer_size);
    snprintf(settings->public_ipv4, buffer_size, "%s=", public_ipv4_var);
    memcpy(settings->public_ipv4 + strlen(public_ipv4_var) + 1, ptr, nmemb);
    settings->public_ipv4[buffer_size - 1] = '\0';

    return nmemb;
}

int main(int argc, char** argv)
{
    CURL *curl;
    CURLcode res;

    AppSettings settings;

    settings.public_ipv4 = strdup("192.168.1.33");

    /* In windows, this will init the winsock stuff */ 
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */ 
    curl = curl_easy_init();
    if(curl) {
    /* First set the URL that is about to receive our POST. This URL can
        just as well be a https:// URL if that is what should receive the
        data. */ 
    curl_easy_setopt(curl, CURLOPT_URL, "http://169.254.169.254/latest/meta-data/public-ipv4");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_public_ip_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &settings);

    /* Perform the request, res will get the return code */ 
    res = CURLE_OK;//curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK) {
        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        printf("%s\n", settings.public_ipv4);
        const char* home_val = getenv(home_env_var);
        size_t buf_size = strlen(home_env_var) + 1 + strlen(home_val) + 1;//HOME=/bla/bla/bla + terminnating 0
        char* home_path_buf = (char*)malloc(buf_size);
        snprintf(home_path_buf, buf_size,"%s=%s", home_env_var, home_val);
        char *env[] = {
            settings.public_ipv4,
            home_path_buf,
            NULL };
        char **new_argv = (char**)malloc((argc)*sizeof(char*));
        int i = 0;
        for (; i < argc - 1; ++i) {
            new_argv[i] = strdup(argv[i + 1]);
        }
        new_argv[i] = NULL;

        pid_t pid = fork();
        if (pid == -1) {
             printf("fork error!");
        } else if (pid == 0) {
            execve(new_argv[0], new_argv, env);
        }

        i = 0;
        for (; i < argc; ++i) {
            free(new_argv[i]);
        }

        free(new_argv);
        free(home_path_buf);
    }

    if (settings.public_ipv4 != NULL) {
        free(settings.public_ipv4);
    }

    /* always cleanup */ 
    curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}