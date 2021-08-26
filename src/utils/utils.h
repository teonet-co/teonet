/*
 * File:   utils.h
 * Author: Kirill Scherba
 *
 * Created on April 10, 2015, 6:13 PM
 */

#ifndef UTILS_H
#define	UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#include "rlutil.h"
#include "string_arr.h"
#include "config/conf.h"

typedef struct addr_port {
    char *addr;
    uint16_t port;
    int (*equal)(struct addr_port *, char*, uint16_t);
} addr_port_t;

addr_port_t *wrap_inet_ntop(const struct sockaddr *sa);
void addr_port_free(addr_port_t *ap_obj);

/**
 * KSNet printf messages types
 */
typedef enum ksnet_printf_type {

            NONE_LOG,     ///< #0 Log off
            ERROR_M,      ///< #1 Error message
            CONNECT,      ///< #2 Connect or Auth messages 
            MESSAGE,      ///< #3 Regular messages
            DEBUG,        ///< #4 Debug message (normal)
            DEBUG_VV,     ///< #5 Debug message (extra)
            DEBUG_VVV,    ///< #6 Debug message (extra+)
            DISPLAY_M,    ///< #7 Regular messages (display only)
                    
} ksnet_printf_type;

#define _ksn_printf_type_(type) \
   (type == MESSAGE ? "MESSAGE" : \
    type == ERROR_M ? "ERROR" : \
    type == DEBUG ? "DEBUG" : \
    type == DEBUG_VV ? "DEBUG_VV" : \
    type == DEBUG_VVV ? "DEBUG_VVV" : \
    type == CONNECT ? "CONNECT" : "DISPLAY")

#define _ksn_printf_format_(format) "%s %s: " _ANSI_GREY "%s:(%s:%d)" _ANSI_NONE ": " _ANSI_GREEN format _ANSI_NONE
#define _ksn_printf_format_display_m(format) "%s%s%s%s%d\b" format

#define ksn_printf(ke, module, type, format, ...) \
    ksnet_printf(&((ke)->teo_cfg), type, \
        _ksn_printf_format_(format), \
        _ksn_printf_type_(type), \
        module[0] == '\0' ? (ke)->teo_cfg.app_name : module, \
        __func__, __FILE__, __LINE__, __VA_ARGS__)

#define ksn_puts(ke, module, type, format) \
    ksnet_printf(&((ke)->teo_cfg), type, \
        _ksn_printf_format_(format) "\n", \
        _ksn_printf_type_(type), \
        module[0] == '\0' ? (ke)->teo_cfg.app_name : module, \
        __func__, __FILE__, __LINE__)


#ifdef	__cplusplus
extern "C" {
#endif

int teoLogPuts(teonet_cfg *teo_cfg, const char* module , int type, const char* message);        
int ksnet_printf(teonet_cfg *teo_cfg, int type, const char* format, ...);       
char *ksnet_formatMessage(const char *fmt, ...);
char *ksnet_sformatMessage(char *str_to_free, const char *fmt, ...);
char *ksnet_vformatMessage(const char *fmt, va_list ap);

char *getRandomHostName(void); // Implemented in enet.c module

void *memdup(const void* d, size_t s);
char *trim(char *str);
char *trimlf(char *str);
char *removeTEsc(char *str);
//char* itoa(int ival);
int calculate_lines(char *str);
int inarray(int val, const int *arr, int size);

int set_reuseaddr(int sd);

char* getDataPath(void);
char *ksnet_getSysConfigDir(void);

ksnet_stringArr getIPs(teonet_cfg *conf);
int ip_is_private(char *ip);
int ip_to_array(char* ip, uint8_t *arr);

size_t get_num_of_tags(char *data, size_t data_length);

void KSN_SET_TEST_MODE(int test_mode);
int KSN_GET_TEST_MODE();


char *ksn_base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length);

unsigned char *ksn_base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);

void dump_bytes(char *buffer, int buffer_len, uint8_t* data, int data_len);

void printHexDump(void *addr, size_t len);
void resolveDnsName(teonet_cfg *conf);
#ifdef	__cplusplus
}
#endif

#endif	/* UTILS_H */
