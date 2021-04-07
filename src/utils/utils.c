#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <libgen.h>
#include <syslog.h>
#include <errno.h>

#include "config/config.h"

#ifdef HAVE_MINGW
#include <ws2tcpip.h>
#include <winsock2.h>
#endif

#include "config/conf.h"
#include "rlutil.h"
#include "utils.h"
#include "ev_mgr.h"


#define ADDRSTRLEN 128

//double ksnetEvMgrGetTime(void *ke);
void teoLoggingClientSend(void *ke, const char *message);
unsigned char teoFilterFlagCheck(void *ke);
unsigned char teoLogCheck(void *ke, void *log);

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
 * @param teo_cfg Pointer to teonet_cfg
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
int ksnet_printf(teonet_cfg *teo_cfg, int type, const char* format, ...) {

    static int log_opened = 0;
    int show_it = 0,
        show_log = 1,
        priority = LOG_USER,
        ret_val = 0;

    // Skip execution in tests
    if(KSN_GET_TEST_MODE()) return ret_val;

    pthread_mutex_lock(&((ksnetEvMgrClass*)(teo_cfg->ke))->printf_mutex);

    // Check type
    switch(type) {

        case CONNECT:

            if(teo_cfg->show_connect_f) show_it = 1;
            priority = LOG_NOTICE; //LOG_AUTH;
            break;

        case DEBUG:

            if(teo_cfg->show_debug_f || teo_cfg->show_debug_vv_f || teo_cfg->show_debug_vvv_f) show_it = 1;
            priority = LOG_INFO;
            break;

        case DEBUG_VV:

            if(teo_cfg->show_debug_vv_f || teo_cfg->show_debug_vvv_f) show_it = 1;
            priority = LOG_DEBUG;
            break;

        case DEBUG_VVV:

            if(teo_cfg->show_debug_vvv_f) show_it = 1;
            priority = LOG_DEBUG;
            break;

        case MESSAGE:
            priority = LOG_NOTICE;
            show_it = 1;
            break;

        case DISPLAY_M:
            priority = LOG_NOTICE;
            show_it = 1;
            show_log = 0;
            break;

        case ERROR_M:
            priority = LOG_ERR;
            show_it = 1;
            break;

        default:
            priority = LOG_INFO;
            show_it = 1;
            break;
    }

    show_log = show_log && (teo_cfg->log_priority >= type);

    if(show_it || show_log) {

        va_list args;
        va_start(args, format);
        char *p = ksnet_vformatMessage(format, args);
        va_end(args);

        // Filter
        if(show_it) {
            if(teoFilterFlagCheck(teo_cfg->ke))
                if(teoLogCheck(teo_cfg->ke, p)) show_it = 1; else show_it = 0;
            else show_it = 0;
        }

        // Show message
        if(show_it) {
            double ct = ksnetEvMgrGetTime(teo_cfg->ke);
            uint64_t raw_time = ct * 1000;
            time_t e_time = raw_time / 1000;
            unsigned ms_time = raw_time % 1000;
            struct tm tm = *localtime(&e_time);

            char t[64];
            strftime(t, sizeof(t), "[%F %T", &tm);

            char timestamp[128];
            snprintf(timestamp, sizeof timestamp, "%s:%03u]", t, ms_time);

            if(type != DISPLAY_M && ct != 0.00)
                printf("%s%s%s ",
                       teo_cfg->color_output_disable_f ? "" : _ANSI_NONE,
                       timestamp,
                       teo_cfg->color_output_disable_f ? "" : _ANSI_NONE
                );
                
            if(teo_cfg->color_output_disable_f) {
                trimlf(removeTEsc(p));
                printf("%s\n", p);
            }
            else printf("%s", p);
            fflush(stdout);
        }

        // Log message
        if(show_log) {

            // Open log at first message
            if(!log_opened) {
                // Open log
                setlogmask (LOG_UPTO (LOG_INFO));
                openlog (teo_cfg->log_prefix, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
                log_opened = 1;
            }

            // Log message
            char *data = trimlf(removeTEsc(p));

            // Save log to syslog
            syslog(priority < LOG_DEBUG ? priority : LOG_INFO, "%s", data);

            // Send async event to teonet event loop (which processing in
            // logging client module) to send log to logging server
            teoLoggingClientSend(teo_cfg->ke, data);
        }
        free(p);
    }

    pthread_mutex_unlock(&((ksnetEvMgrClass*)(teo_cfg->ke))->printf_mutex);

    return ret_val;
}


int teoLogPuts(teonet_cfg *teo_cfg, const char* module , int type, const char* message) {
    return ksnet_printf(teo_cfg, type,
        "%s %s: " /*_ANSI_GREY "%s:(%s:%d)" _ANSI_NONE ": "*/ _ANSI_GREEN "%s" _ANSI_NONE "\n",
        _ksn_printf_type_(type),
        module == NULL || module[0] == 0 ? teo_cfg->app_name : module,
        /*"", "", "",*/ message);
}

/**
 * Remove terminal escape substrings from string
 *
 * @param str
 * @return
 */
char *removeTEsc(char *str) {

    int i, j = 0, skip_esc = 0, len = strlen(str);
    for(i = 0; i <= len; i++) {

        if(skip_esc && str[i] == 'm') skip_esc = 0;
        else if(!skip_esc && str[i] == '\033') skip_esc = 1;
        else if(!skip_esc) str[j++] = str[i];
    }

    return str;
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

    if(str_to_free != NULL) {
        free(str_to_free);
    }

    return p;
}

char *ksnet_vformatMessage(const char *fmt, va_list ap) {

    int size = KSN_BUFFER_SM_SIZE; /* Guess we need no more than 100 bytes */
    char *p, *np;
    va_list ap_copy;
    int n;

    if((p = malloc(size)) == NULL) {
        return NULL;
    }

    while(1) {

        // Try to print in the allocated space
        va_copy(ap_copy,ap);
        n = vsnprintf(p, size, fmt, ap_copy);
        va_end(ap_copy);

        // Check error code
        if(n < 0)
            return NULL;

        // If that worked, return the string
        if(n < size) {
            return p;
        }

        // Else try again with more space
        size = n + KSN_BUFFER_SM_SIZE; // Precisely what is needed
        if((np = realloc(p, size)) == NULL) {
            free(p);
            return NULL;
        } else {
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

/**
 * Duplicate memory \todo move this function to teonet library
 *
 * Allocate memory and copy selected value to it
 *
 * @param d Pointer to value to copy
 * @param s Length of value
 * @return
 */
void *memdup(const void* d, size_t s) {

    void* p;
    return ((p = malloc(s))?memcpy(p, d, s):NULL);
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
 * Get path to teonet data folder
 *
 * @return NULL terminated static string
 */
char* getDataPath(void) {

    char* dataDir = NULL;

    if(dataDir == NULL) {

//#define DATA_DIR ".ksnet"
#ifdef HAVE_LINUX
//#define _GNU_SOURCE
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

/**
 * Make socket reusable
 *
 * @param sd
 * @return
 */
int set_reuseaddr(int sd) {

    // Make socket reusable
    int yes = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        return -1;
    }
    return 0;
}
#endif

/**
 * Getting path to ksnet configuration folder
 *
 * @return NULL terminated string
 */
char *ksnet_getSysConfigDir(void) {

    char* sysConfigDir = strdup(TEONET_SYS_CONFIG_DIR);

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
ksnet_stringArr getIPs(teonet_cfg *conf) {

    ksnet_stringArr arr = ksnet_stringArrCreate();

    const int CHECK_IPv6 = 0;

    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;

    #ifndef HAVE_MINGW
    getifaddrs(&ifAddrStruct);

    for(ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next ) {

        // Skip interface with empty address
        if(!ifa ->ifa_addr) continue;

        // Check it is IP4
        if(ifa ->ifa_addr->sa_family == AF_INET) {

            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //printf("%s IP Address: %s\n", ifa->ifa_name, addressBuffer);

            // Skip VPN IP
            if(!strcmp(addressBuffer, conf->vpn_ip)) continue;

            ksnet_stringArrAdd(&arr, addressBuffer);
        }
        // Check it is IP6
        else if(CHECK_IPv6 && ifa->ifa_addr->sa_family == AF_INET6) {
            // is a valid IP6 Address
            tmpAddrPtr = &((struct sockaddr_in6 *) ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
            ksnet_stringArrAdd(&arr, addressBuffer);
        }
    }

    if(ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);
    #endif

    return arr;
}

int addr_port_equal(addr_port_t *ap_obj, char *addr, uint16_t port) {
    if (ap_obj->port == port && !strcmp(ap_obj->addr, addr)) {
        return 1;
    }

    return 0;
}

addr_port_t *addr_port_init() {
    addr_port_t *ptr = malloc(sizeof(addr_port_t));
    ptr->addr = malloc(ADDRSTRLEN);
    ptr->addr[0] = '\0';
    ptr->port = 0;
    ptr->equal = addr_port_equal;
    return ptr;
}

void addr_port_free(addr_port_t *ap_obj) {
    free(ap_obj->addr);
    free(ap_obj);
}

addr_port_t *wr__ntop(const struct sockaddr *sa) {
    addr_port_t *ptr = addr_port_init();

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, ptr->addr, ADDRSTRLEN) == NULL) {
            printf("inet_ntop ipv4 conversion error: %s\n", strerror(errno));
			return NULL;
        }
		if (ntohs(sin->sin_port) != 0) {
            ptr->port = ntohs(sin->sin_port);
		}

		return ptr;
	}

	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		if (inet_ntop(AF_INET6, &sin6->sin6_addr, ptr->addr, ADDRSTRLEN) == NULL) {
            printf("inet_ntop ipv6 conversion error: %s\n", strerror(errno));
			return NULL;
        }
		if (ntohs(sin6->sin6_port) != 0) {
            ptr->port = ntohs(sin6->sin6_port);
		}

		return ptr;
	}

	default:
		return NULL;
	}
    return NULL;
}

addr_port_t *wrap_inet_ntop(const struct sockaddr *sa) {
    addr_port_t *ptr;
	if ( (ptr = wr__ntop(sa)) == NULL) {
        fprintf(stderr, "wrap_inet_ntop error\n");
        exit(1);
    }

    return ptr;
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


/*
    Example of output printHexDump function for connect_r_packet_t struct

  0000  8a 1b 00 00 04 22 74 65   6f 2d 76 70 6e 22 2c 20    ....."teo-vpn", 
  0010  22 74 65 6f 2d 6c 30 22   00 00 00 00 00 00 00 00    "teo-l0"........
  0020  00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00    ................
  0030  00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00    ................
  0040  00 00 00 00 00 31 39 32   2e 31 36 38 2e 31 2e 36    .....192.168.1.6
  0050  39 00 31 39 32 2e 31 36   38 2e 31 32 32 2e 31 00    9.192.168.122.1.
  0060  31 37 32 2e 31 37 2e 30   2e 31 00 31 30 2e 31 33    172.17.0.1.10.13
  0070  35 2e 31 34 32 2e 38 33   00 00 00 00 00 00 00 00    5.142.83........
  0080  00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00    ................
  0090  00 00 00 00 00                                       .....
*/
void printHexDump(void *addr, size_t len)  {
    unsigned char buf[17];
    unsigned char *pc = addr;
    size_t i = 0;
    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0) {
                printf("  %s\n", buf);
            }

            // print offset.
            printf("  %04lx ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);
        if (((i+1) % 8) == 0) {
            printf("  ");
        }

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            buf[i % 16] = '.';
        } else {
            buf[i % 16] = pc[i];
        }

        buf[(i % 16) + 1] = '\0';
    }


    while ((i % 16) != 0) {
        if (((i+1) % 8) == 0) {
            printf("  ");
        }
        printf("   ");
        i++;
    }

    printf("  %s\n", buf);
}

void resolveDnsName(teonet_cfg *conf) {
    memset(conf->r_host_addr, '\0', sizeof(conf->r_host_addr));

    const char *localhost_str = "localhost";
    if (!strncmp(localhost_str, conf->r_host_addr_opt, strlen(localhost_str))) {
        const char *localhost_num = "::1";
        strncpy((char*)conf->r_host_addr, localhost_num, strlen(localhost_num));
    } else {
        struct addrinfo hint;
        struct addrinfo *res = NULL;
        memset(&hint, '\0', sizeof hint);

        hint.ai_family = PF_UNSPEC;

        int ret = getaddrinfo(conf->r_host_addr_opt, NULL, &hint, &res);
        if (ret) {
            fprintf(stderr, "Invalid address. %s\n", gai_strerror(ret));
            exit(1);
        }

        addr_port_t *ap_obj = wrap_inet_ntop(res->ai_addr);
        strncpy((char*)conf->r_host_addr, ap_obj->addr, KSN_BUFFER_SM_SIZE/2);
        addr_port_free(ap_obj);
        freeaddrinfo(res);
    }
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

/**
 * Calculate number of tags in json string
 *
 * @param data
 * @param data_length
 * @return
 */
size_t get_num_of_tags(char *data, size_t data_length) {

    int i = 0;
    size_t num_of_tags = 0;

    for(i = 0; i < data_length; i++)
        if(data[i] == ':')
            num_of_tags++;

    num_of_tags++;

    return num_of_tags * 4;
}
