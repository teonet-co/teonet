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
    char *public_ipv4;
    char *public_ipv6;
} AppSettings;

size_t save_data_cb(void *ptr, size_t size, size_t nmemb, void *stream){
    int buffer_size = nmemb + 1;
    char** buffer_ptr = (char**)stream;

    *buffer_ptr = (char*)malloc(buffer_size);
    memcpy(*buffer_ptr, ptr, nmemb);
    (*buffer_ptr)[buffer_size - 1] = '\0';

    return nmemb;
}

int request_public_ipv4(CURL *curl, AppSettings *settings) {
    char *buffer = NULL;

    curl_easy_setopt(curl, CURLOPT_URL, "http://169.254.169.254/latest/meta-data/public-ipv4");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_data_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        printf("request_public_ipv4 failed: %s\n", curl_easy_strerror(res));
        return 0;
    }

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (response_code == 200) {
        settings->public_ipv4 = strdup(buffer);
    } else {
        settings->public_ipv4 = NULL;
    }

    free(buffer);

    return 1;
}

int request_public_ipv6(CURL *curl, AppSettings *settings) {
    char *buffer = NULL;
    curl_easy_setopt(curl, CURLOPT_URL, "http://169.254.169.254/latest/meta-data/network/interfaces/macs/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_data_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        printf("request macs failed: %s\n", curl_easy_strerror(res));
        return 0;
    }

    char *ipv6_url_request_buffer = NULL;
    const char* ipv6_url_pattern = "http://169.254.169.254/latest/meta-data/network/interfaces/macs/ipv6s";
    //NOTE: buffer holds string like 02:43:8a:02:50:57/, so it already has '/'
    size_t ipv6_url_buffer_size = strlen(ipv6_url_pattern) + strlen(buffer) + 1;
    ipv6_url_request_buffer = (char*)malloc(ipv6_url_buffer_size);//http://169.254.169.254/latest/meta-data/network/interfaces/macs/mac-address/ipv6s
    snprintf(ipv6_url_request_buffer, ipv6_url_buffer_size,
        "http://169.254.169.254/latest/meta-data/network/interfaces/macs/%sipv6s", buffer);
    free(buffer);

    buffer = NULL;

    printf("%s\n", ipv6_url_request_buffer);

    curl_easy_setopt(curl, CURLOPT_URL, ipv6_url_request_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_data_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("request_public_ipv6 failed: %s\n", curl_easy_strerror(res));
        free(ipv6_url_request_buffer);
        return 0;
    }

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    printf("%s\n%ld\n", buffer, response_code);

    if (response_code == 200) {
        settings->public_ipv6 = strdup(buffer);
    } else {
        settings->public_ipv6 = NULL;
    }

    free(ipv6_url_request_buffer);
    free(buffer);

    return 1;
}

void launch_app(AppSettings* settings, int argc, char** argv) {
    int has_public_ipv4_addr = settings->public_ipv4 != NULL;
    int has_public_ipv6_addr = settings->public_ipv6 != NULL;
    assert((has_public_ipv4_addr || has_public_ipv6_addr) && "public ip not set");

    printf("%s\n%s\n", settings->public_ipv4, settings->public_ipv6);

    const char* home_val = getenv(home_env_var);
    printf("%s\n", home_val);
    size_t buf_size = strlen(home_env_var) + 1 + strlen(home_val) + 1;//HOME=/bla/bla/bla + terminnating 0
    char* home_path_buf = (char*)malloc(buf_size);
    snprintf(home_path_buf, buf_size,"%s=%s", home_env_var, home_val);
    char *env[] = {
        home_path_buf,
        NULL };
    size_t arg_num = argc;
    if (has_public_ipv4_addr) {
        arg_num += 2;//--l0_public_ipv4 + ip
    }

    if (has_public_ipv6_addr) {
        arg_num += 2;//--l0_public_ipv6 + ip
    }

    char **new_argv = (char**)malloc((arg_num)*sizeof(char*));
    int i = 0;
    for (; i < argc - 1; ++i) {
        new_argv[i] = strdup(argv[i + 1]);
    }

    if (has_public_ipv4_addr) {
        new_argv[i] = strdup("--l0_public_ipv4");
        new_argv[i + 1] = strdup(settings->public_ipv4);
        i += 2;
    }

    if (has_public_ipv6_addr) {
        new_argv[i] = strdup("--l0_public_ipv6");
        new_argv[i + 1] = strdup(settings->public_ipv6);
        i += 2;
    }

    new_argv[i] = NULL;

    printf("%s\n", new_argv[0]);

    execve(new_argv[0], new_argv, env);
    perror("execve");
    i = 0;
    for (; i < arg_num; ++i) {
        free(new_argv[i]);
    }

    free(new_argv);
    free(home_path_buf);
}

int main(int argc, char** argv)
{
    char* buf = (char*)malloc(1024);
    memset(buf, '\0', 1024);

    readlink("/proc/self/exe", buf, 1024);

    printf("%s\n", buf);

    free(buf);

    CURL *curl = NULL;

    AppSettings settings;

    settings.public_ipv4 = NULL;
    settings.public_ipv6 = NULL;

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        /* First set the URL that is about to receive our POST. This URL can
            just as well be a https:// URL if that is what should receive the
            data. */
        int result = request_public_ipv4(curl, &settings);
        if (result) {
            result = request_public_ipv6(curl, &settings);
            if (result) {
                launch_app(&settings, argc, argv);
            }
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    if (settings.public_ipv4 != NULL) {
        free(settings.public_ipv4);
    }

    if (settings.public_ipv6 != NULL) {
        free(settings.public_ipv6);
    }

    curl_global_cleanup();
    return 0;
}