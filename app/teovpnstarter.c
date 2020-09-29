#include <curl/curl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

static const char* home_env_var = "HOME";

typedef struct AppSettings
{
    char* public_ipv4;
} AppSettings;

size_t save_public_ip_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
    AppSettings* settings = (AppSettings*)stream;
    size_t buffer_size = nmemb + 1; //111.111.111.111 + terminating 0
    settings->public_ipv4 = (char*)malloc(buffer_size);
    memcpy(settings->public_ipv4, ptr, nmemb);
    settings->public_ipv4[buffer_size - 1] = '\0';
    return nmemb;
}

int main(int argc, char** argv)
{
    char* buf = (char*)malloc(1024);
    memset(buf, '\0', 1024);

    readlink("/proc/self/exe", buf, 1024);

    printf("%s\n", buf);

    free(buf);

    CURL *curl;
    CURLcode res;

    AppSettings settings;

    settings.public_ipv4 = NULL;

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
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        assert(settings.public_ipv4 != NULL && "public ipv4 not set");
        const char* home_val = getenv(home_env_var);
        printf("%s\n", home_val);
        size_t buf_size = strlen(home_env_var) + 1 + strlen(home_val) + 1;//HOME=/bla/bla/bla + terminnating 0
        char* home_path_buf = (char*)malloc(buf_size);
        snprintf(home_path_buf, buf_size,"%s=%s", home_env_var, home_val);
        char *env[] = {
            home_path_buf,
            NULL };
        char **new_argv = (char**)malloc((argc + 2)*sizeof(char*));//args of starting app + public_ipv4 param
        int i = 0;
        for (; i < argc - 1; ++i) {
            new_argv[i] = strdup(argv[i + 1]);
        }

        new_argv[i] = strdup("--l0_public_ipv4");
        new_argv[i + 1] = strdup(settings.public_ipv4);
        new_argv[i + 2] = NULL;

        printf("%s\n", new_argv[0]);

        execve(new_argv[0], new_argv, env);
        perror("execve");
        i = 0;
        for (; i < argc + 2; ++i) {
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