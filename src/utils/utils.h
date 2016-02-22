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

#include "string_arr.h"
#include "config/conf.h"

/**
 * KSNet printf types
 */
typedef enum ksnet_printf_type {

            MESSAGE = 0,  ///< Regular message
            ERROR_M,      ///< Error message
            DEBUG,        ///< Debug message (normal)
            DEBUG_VV,     ///< Debug message (extra)
            CONNECT,      ///< Connect or Auth message 
            DISPLAY       ///< Regular message (display only)

} ksnet_printf_type;

#define _ksn_printf_type_(type) \
   (type == MESSAGE ? "MESSAGE" : \
    type == ERROR_M ? "ERROR" : \
    type == DEBUG ? "DEBUG" : \
    type == DEBUG_VV ? "DEBUG_VV" : \
    type == CONNECT ? "CONNECT" : "DISPLAY")

#define _ksn_printf_format_(format) "%s %s:" _ANSI_GREY "%s:(%s:%d)" _ANSI_NONE ": " format

#define ksn_printf(ke, module, type, format, ...) \
    ksnet_printf(&ke->ksn_cfg, type, \
        _ksn_printf_format_(format), \
        _ksn_printf_type_(type), \
        module == NULL ? ke->ksn_cfg.app_name : module, \
        __func__, __FILE__, __LINE__, __VA_ARGS__)

#define ksn_puts(ke, module, type, format) \
    ksnet_printf(&ke->ksn_cfg, type, \
        _ksn_printf_format_(format) "\n", \
        _ksn_printf_type_(type), \
        module == NULL ? ke->ksn_cfg.app_name : module, \
        __func__, __FILE__, __LINE__)


#ifdef	__cplusplus
extern "C" {
#endif
    
int ksnet_printf(ksnet_cfg *ksn_cfg, int type, const char* format, ...);
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

void set_nonblock(int sd);
int set_reuseaddr(int sd);

const char* getDataPath(void);
const char *ksnet_getSysConfigDir(void);

ksnet_stringArr getIPs(ksnet_cfg *conf);
int ip_is_private(char *ip);
int ip_to_array(char* ip, uint8_t *arr);

size_t get_num_of_tags(char *data, size_t data_length);

void KSN_SET_TEST_MODE(int test_mode);
int KSN_GET_TEST_MODE();

#ifdef	__cplusplus
}
#endif

#endif	/* UTILS_H */
