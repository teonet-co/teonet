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

typedef struct Buffer
{
    char *data;
    size_t size;
    size_t capacity;
} Buffer;

Buffer create_buffer() {
    Buffer buf;

    buf.data = NULL;
    buf.size = 0;
    buf.capacity = 0;

    return buf;
}

void destroy_buffer(Buffer *buffer) {
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

void clear_buffer(Buffer *buffer) {
    buffer->size = 0;
}

char* stealFromBuffer(Buffer *buffer) {
    char* data = buffer->data;

    buffer->data = NULL;
    buffer->capacity = 0;
    buffer->size = 0;

    return data;
}

void set_buffer(Buffer *buffer, const char* data, size_t data_size) {
    if (buffer == NULL || data == NULL || data_size == 0) { return; }

    free(buffer->data);

    buffer->data = (char*)malloc(data_size);
    memcpy(buffer->data, data, data_size);
    buffer->size = data_size;
    buffer->capacity = data_size;
}

void append_to_buffer(Buffer *buffer, const char* data, size_t data_size) {
    if (buffer == NULL || data == NULL || data_size == 0) { return; }

    if (buffer->data == NULL) {//NOTE: empty buffer, just copy data
        set_buffer(buffer, data, data_size);
    } else {
        size_t old_capacity = buffer->capacity;
        while(buffer->capacity - buffer->size < data_size) {
            buffer->capacity *= 2;
        }

        if (old_capacity != buffer->capacity) {
            //TOOD: realloc can fail
            buffer->data = (char*)realloc(buffer->data, buffer->capacity);
        }

        memcpy(buffer->data + buffer->size, data, data_size);
        buffer->size += data_size;
    }
}

size_t save_data_cb(void *ptr, size_t size, size_t nmemb, void *stream){
    Buffer *buffer = (Buffer*)stream;

    append_to_buffer(buffer, ptr, nmemb);

    return nmemb;
}

int request_public_ipv4(CURL *curl, AppSettings *settings) {
    Buffer data_buffer = create_buffer();

    curl_easy_setopt(curl, CURLOPT_URL, "http://169.254.169.254/latest/meta-data/public-ipv4");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_data_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data_buffer);

    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        printf("request_public_ipv4 failed: %s\n", curl_easy_strerror(res));
        destroy_buffer(&data_buffer);
        return 0;
    }

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (response_code == 200) {
        append_to_buffer(&data_buffer, "", 1);//NOTE: add terminating null
        settings->public_ipv4 = stealFromBuffer(&data_buffer);
    } else {
        settings->public_ipv4 = NULL;
    }

    destroy_buffer(&data_buffer);

    return 1;
}

int request_public_ipv6(CURL *curl, AppSettings *settings) {
    Buffer data_buffer = create_buffer();
    curl_easy_setopt(curl, CURLOPT_URL, "http://169.254.169.254/latest/meta-data/network/interfaces/macs/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_data_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data_buffer);

    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        printf("request macs failed: %s\n", curl_easy_strerror(res));
        destroy_buffer(&data_buffer);
        return 0;
    }

    //TODO:check http code

    char *ipv6_url_request_buffer = NULL;
    const char* ipv6_url_pattern = "http://169.254.169.254/latest/meta-data/network/interfaces/macs/ipv6s";
    //NOTE: buffer holds string like 02:43:8a:02:50:57/, so it already has '/'
    size_t ipv6_url_buffer_size = strlen(ipv6_url_pattern) + data_buffer.size + 1;
    append_to_buffer(&data_buffer, "", 1);//NOTE: add terminating null
    ipv6_url_request_buffer = (char*)malloc(ipv6_url_buffer_size);//http://169.254.169.254/latest/meta-data/network/interfaces/macs/mac-address/ipv6s
    snprintf(ipv6_url_request_buffer, ipv6_url_buffer_size,
        "http://169.254.169.254/latest/meta-data/network/interfaces/macs/%sipv6s", data_buffer.data);
    clear_buffer(&data_buffer);

    printf("%s\n", ipv6_url_request_buffer);

    curl_easy_setopt(curl, CURLOPT_URL, ipv6_url_request_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_data_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data_buffer);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        printf("request_public_ipv6 failed: %s\n", curl_easy_strerror(res));
        free(ipv6_url_request_buffer);
        destroy_buffer(&data_buffer);
        return 0;
    }

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    printf("%ld\n", response_code);

    if (response_code == 200) {
        append_to_buffer(&data_buffer, "", 1);//NOTE: add terminating null
        settings->public_ipv6 = stealFromBuffer(&data_buffer);
    } else {
        settings->public_ipv6 = NULL;
    }

    free(ipv6_url_request_buffer);
    destroy_buffer(&data_buffer);

    return 1;
}

void launch_app(AppSettings* settings, int argc, char** argv) {
    int has_public_ipv4_addr = settings->public_ipv4 != NULL;
    int has_public_ipv6_addr = settings->public_ipv6 != NULL;
    assert((has_public_ipv4_addr || has_public_ipv6_addr) && "public ip not set");

    printf("%s\n%s\n", settings->public_ipv4, settings->public_ipv6);

    const char* home_val = getenv(home_env_var);
    printf("%s\n", home_val);
    size_t buf_size = strlen(home_env_var) + 1 + strlen(home_val) + 1;//HOME=/bla/bla/bla + terminating 0
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
        new_argv[i + 1] = settings->public_ipv4;
        settings->public_ipv4 = NULL;
        i += 2;
    }

    if (has_public_ipv6_addr) {
        new_argv[i] = strdup("--l0_public_ipv6");
        new_argv[i + 1] = settings->public_ipv6;
        settings->public_ipv6 = NULL;
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