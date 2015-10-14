#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <libgen.h>

#include "config/config.h"

#ifdef HAVE_MINGW
#include <ws2tcpip.h>
#include <winsock2.h>
#endif

#include "config/conf.h"
#include "utils.h"
#include "rlutil.h"

double ksnetEvMgrGetTime(void *ke);

// Test mode (for tests only)
static int KSN_TEST_MODE = 0;
inline void KSN_SET_TEST_MODE(int test_mode) {
    KSN_TEST_MODE = test_mode;
}
inline int KSN_GET_TEST_MODE() {
    return KSN_TEST_MODE;
}

/**
 * KSNet printf. @see vprintf
 *
 * Make format output to stdout. Instead of standard printf function this
 * function has type parameter which define type of print. It should be used
 * this function instead of standard printf function.
 *
 * @param ksn_cfg Pointer to ksnet_cfg
 * @param type MESSAGE -- print always;
 *             CONNECT -- print if connect show flag  connect is set;
 *             DEBUG -- print if debug show flag is set on;
 *             DEBUG_VV -- print if debug extra show flag is set on;
 *             ERROR_M -- print always.
 * @param format Format like in standard printf function
 * @param ... Parameters
 *
 * @return Number of byte printed
 */
int ksnet_printf(ksnet_cfg *ksn_cfg, int type, const char* format, ...) {

    int show_it = 0, ret_val = 0;
    
    // Skip execution in tests
    if(KSN_GET_TEST_MODE()) return ret_val;

    switch(type) {

        case CONNECT:

            if(ksn_cfg->show_connect_f) show_it = 1;
            break;

        case DEBUG:

            if(ksn_cfg->show_debug_f) show_it = 1;
            break;

        case DEBUG_VV:

            if(ksn_cfg->show_debug_vv_f) show_it = 1;
            break;

        case MESSAGE:
        case ERROR_M:
        default:
            show_it = 1;
    }

    if(show_it) {

        double ct = ksnetEvMgrGetTime(ksn_cfg->ke);
        if(type != MESSAGE && ct != 0.00)
            printf("%s%f:%s ", ANSI_DARKGREY, ct, ANSI_NONE);

        va_list args;
        va_start(args, format);
        ret_val = vprintf(format, args);
        va_end(args);
    }

    return ret_val;
}

/**
 * Create formated message in new null terminated string
 *
 * @param fmt Format string like in printf function
 * @param ... Parameter
 *
 * @return Null terminated string, should be free after using or NULL on error
 */
char *ksnet_formatMessage(const char *fmt, ...) {

    va_list ap;
    va_start(ap, fmt);
    char *p = ksnet_vformatMessage(fmt, ap);
    va_end(ap);

    return p;
}
/**
 * Create formated message in new null terminated string, and free str_to_free
 *
 * @param str_to_free
 * @param fmt Format string like in printf function
 * @param ... Parameter
 *
 * @return Null terminated string, should be free after using or NULL on error
 */
char *ksnet_sformatMessage(char *str_to_free, const char *fmt, ...) {

    va_list ap;
    va_start(ap, fmt);
    char *p = ksnet_vformatMessage(fmt, ap);
    va_end(ap);

    if(str_to_free != NULL) free(str_to_free);

    return p;
}

char *ksnet_vformatMessage(const char *fmt, va_list ap) {

    int size = KSN_BUFFER_SM_SIZE; /* Guess we need no more than 100 bytes */
    char *p, *np;
    va_list ap_copy;
    int n;

    if((p = malloc(size)) == NULL)
        return NULL;

    while(1) {

        // Try to print in the allocated space
        va_copy(ap_copy,ap);
        n = vsnprintf(p, size, fmt, ap_copy);
        va_end(ap_copy);

        // Check error code
        if(n < 0)
            return NULL;

        // If that worked, return the string
        if(n < size)
            return p;

        // Else try again with more space
        size = n + KSN_BUFFER_SM_SIZE; // Precisely what is needed
        if((np = realloc(p, size)) == NULL) {
            free(p);
            return NULL;
        }
        else {
            p = np;
        }
    }
}

/**
 * Remove trailing line feeds characters from null
 * terminated string
 *
 * @param str
 * @return
 */
char *trimlf(char *str) {

    // Trim at the end of string
    int len = strlen(str);
    while(len && (str[len - 1] == '\n' || str[len - 1] == '\r'))
        str[(--len)] = '\0';

    return str;
}

/**
 * Remove trailing or leading space
 *
 * Remove trailing or leading space or tabulation characters in
 * input null terminated string
 *
 * @param str Null terminated input string to remove trailing and leading
 *            spaces in
 * @return Pointer to input string (the same as input string)
 */
char *trim(char *str) {

    int i, j = 0, stop = 0;

    // Trim at begin of string
    for(i = 0; str[i] != '\0'; i++) {
        if(stop || (str[i] != ' ' && str[i] != '\t')) {
            str[j++] = str[i];
            stop = 1;
        }
    }
    str[j] = '\0';

    // Trim at the end of string
    int len = strlen(str);
    while(len && (str[len - 1] == ' ' || str[len - 1] == '\t'))
        str[(len--) - 1 ] = '\0';

    return str;
}

/*
 * Convert integer to string
 *
 * param ival
 * return
 */
//char* itoa(int ival) {
//
//    char buffer[KSN_BUFFER_SIZE];
//    snprintf(buffer, KSN_BUFFER_SIZE, "%d", ival);
//
//    return strdup(buffer);
//}

/**
 * Calculate number of lines in string
 *
 * Calculate number of line with line feed \\n at the end in null
 * input terminated string
 *
 * @param str Input null terminated string
 * @return Number of line with linefeed \\n at the end
 */
int calculate_lines(char *str) {

    int i, num = 0;
    for(i = 0; str[i] != 0; i++)
        if(str[i] == '\n') num++;

    return num;
}

/**
 * Get random host name
 * 
 * Create random host name. Should be free after use.
 *
 * @return String with random host name. Should be free after use.
 */
char *getRandomHostName(void) {

    srand(time(NULL));
    int r = rand() % 1000;

    return ksnet_formatMessage("ksnet-%03d", r);
}

/**
 * Get path to ksnet data folder
 *
 * @return NULL terminated static string
 */
const char* getDataPath(void) {

    static char* dataDir = NULL;

    if(dataDir == NULL) {

//#define DATA_DIR ".ksnet"
#ifdef HAVE_LINUX
#define _GNU_SOURCE
#include <errno.h>
#include <libgen.h>
extern char *program_invocation_name;
//extern char *program_invocation_short_name;
char *full_progname = strdup(program_invocation_name);
char *DATA_DIR = ksnet_formatMessage(".%s", basename(full_progname));
free(full_progname);
#else
#ifdef HAVE_MINGW
//#include <windows.h>

//char szExeFileName[KSN_BUFFER_SM_SIZE] = "ksnet";
//GetModuleFileName(NULL, szExeFileName, KSN_BUFFER_SM_SIZE);
//szExeFileName = "ksnet";
char *DATA_DIR = strdup(getenv("APPDATA")); //ksnet_formatMessage(".%s", basename(szExeFileName));
#else
char *DATA_DIR = ksnet_formatMessage(".%s", getprogname());
#endif
#endif

        if(!strncmp(DATA_DIR, ".lt-", 4)) {
            memmove(DATA_DIR + 1, DATA_DIR + 4, strlen(DATA_DIR) + 1 - 4);
        }

//#if RELEASE_KSNET
        char buf[KSN_BUFFER_SIZE];
        strncpy(buf, getenv("HOME"), KSN_BUFFER_SIZE);
        strncat(buf, "/", KSN_BUFFER_SIZE - strlen(buf) - 1);
        strncat(buf, DATA_DIR, KSN_BUFFER_SIZE - strlen(buf) - 1);
        dataDir = strdup(buf);
//#else
//        dataDir = strdup(DATA_DIR);
//#endif
        free(DATA_DIR);
    }

    return dataDir;
}

#if NOT_USED_YET
char *getExecPath (char *path, size_t dest_len, char *argv0) {

    char *baseName = NULL;
    char *systemPath = NULL;
    char *candidateDir = NULL;

    // the easiest case: we are in linux
    if (readlink ("/proc/self/exe", path, dest_len) != -1) {

        dirname(path);
        strcat(path, "/");
        return path;
    }

    // Ups... not in linux, no  guarantee

    // check if we have something like execve("foobar", NULL, NULL)
    if (argv0 == NULL) {

        // we surrender and give current path instead
        if(getcwd(path, dest_len) == NULL ) return NULL;
        strcat(path, "/");
        return path;
    }


    // argv[0]
    // if dest_len < PATH_MAX may cause buffer overflow
    if ((realpath (argv0, path)) && (!access (path, F_OK))) {

        dirname(path);
        strcat(path, "/");
        return path;
    }

    // Current path
    baseName = basename(argv0);
    if (getcwd (path, dest_len - strlen (baseName) - 1) == NULL)
        return NULL;

    strcat(path, "/");
    strcat(path, baseName);
    if (access (path, F_OK) == 0) {

        dirname (path);
        strcat  (path, "/");
        return path;
    }

    // Try the PATH.
    systemPath = getenv ("PATH");
    if (systemPath != NULL) {

        dest_len--;
        systemPath = strdup (systemPath);
        for (candidateDir = strtok (systemPath, ":"); candidateDir != NULL; candidateDir = strtok (NULL, ":"))
        {
            strncpy (path, candidateDir, dest_len);
            strncat (path, "/", dest_len);
            strncat (path, baseName, dest_len);

            if (access(path, F_OK) == 0)
            {
                free (systemPath);
                dirname (path);
                strcat  (path, "/");
                return path;
            }
        }
        free(systemPath);
        dest_len++;
    }

    // again someone has use execve: we dont knowe the executable name; we surrender and give instead current path
    if (getcwd (path, dest_len - 1) == NULL) return NULL;
    strcat  (path, "/");
    return path;
}
#endif

#ifndef HAVE_MINGW
// Moved to teonet_lo_client.c
///**
// * Set socket or FD to non blocking mode
// */
//void set_nonblock(int fd) {
//
//    int flags;
//
//    flags = fcntl(fd, F_GETFL, 0);
//    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
//}
#endif

/**
 * Getting path to ksnet configuration folder
 *
 * @return NULL terminated static string
 */
const char *ksnet_getSysConfigDir(void) {

    static char* sysConfigDir = NULL;

    if(sysConfigDir == NULL) {

#define LOCAL_CONFIG_DIR "src/conf"

//#if RELEASE_KSNET
        sysConfigDir = strdup(TEONET_SYS_CONFIG_DIR);
//#else
//        sysConfigDir = strdup(LOCAL_CONFIG_DIR);
//#endif
    }

    return sysConfigDir;
}

/**
 * Check if a value exist in a array
 * 
 * @param val Integer value
 * @param arr Integer array
 * @param size Array size
 * 
 * @return 
 */
int inarray(int val, const int *arr, int size) {
    
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return 1;
    }
    return 0;
}


#include <sys/types.h>
#ifdef HAVE_MINGW
#include <winsock2.h>
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <string.h>

/**
 * Get IPs
 * 
 * Get IP address of this host
 *
 * @return Pointer to ksnet_stringArr
 */
ksnet_stringArr getIPs() {

    ksnet_stringArr arr = ksnet_stringArrCreate();

    const int CHECK_IPv6 = 0;

    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;

    #ifndef HAVE_MINGW
    getifaddrs(&ifAddrStruct);

    for(ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next ) {

        // Check it is IP4
        if(ifa ->ifa_addr->sa_family == AF_INET) {

            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
//            printf("%s IP Address: %s\n", ifa->ifa_name, addressBuffer);

            // Skip VPN IP
//            if(!strcmp(addressBuffer, conf->vpn_ip)) continue;

            ksnet_stringArrAdd(&arr, addressBuffer);
        }
        // Check it is IP6
        else if(CHECK_IPv6 && ifa->ifa_addr->sa_family == AF_INET6) {
            // is a valid IP6 Address
            tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
            ksnet_stringArrAdd(&arr, addressBuffer);
        }
    }

    if(ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
    #endif

    return arr;
}

/**
 * Convert IP to bytes array
 *
 * @param ip
 * @param arr
 * @return
 */
int ip_to_array(char* ip, uint8_t *arr) {
    int i = 0;

    char *buff, *str_ip = strdup(ip);
    buff = strtok(str_ip, ".");
    while (buff != NULL && i < 4) {

       //if you want to print its value
       //printf("%s\n",buff);
       //and also if you want to save each byte
       arr[i] = (unsigned char)atoi(buff);
       buff = strtok(NULL,".");
       i++;
    }
    free(str_ip);

    return i;
}

/**
 * Detect if input IP address is private
 *
 * @param ip
 * @return
 */
int ip_is_private(char *ip) {

    uint8_t ip_arr[4];

    ip_to_array(ip, ip_arr);

    if(ip_arr[0] == 10 ||
       (ip_arr[0] == 192 && ip_arr[1] == 168) ||
       (ip_arr[0] == 172 && ip_arr[1] >= 16 && ip_arr[1] <= 31) ) return 1;

    else return 0;
}
