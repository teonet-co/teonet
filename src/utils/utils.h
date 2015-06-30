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

            MESSAGE = 0,
            ERROR_M,
            DEBUG,
            DEBUG_VV,
            CONNECT

} ksnet_printf_type;

#ifdef	__cplusplus
extern "C" {
#endif

int ksnet_printf(ksnet_cfg *ksn_cfg, int type, const char* format, ...);
char *ksnet_formatMessage(const char *fmt, ...);
char *ksnet_sformatMessage(char *str_to_free, const char *fmt, ...);
char *ksnet_vformatMessage(const char *fmt, va_list ap);

char *getRandomHostName(void); // Implemented in enet.c module

char *trim(char *str);
char *trimlf(char *str);
//char* itoa(int ival);
int calculate_lines(char *str);

void set_nonblock(int fd);

const char* ksnet_getDataDir(void);
const char *ksnet_getSysConfigDir(void);

ksnet_stringArr getIPs(/*ksnet_config *conf*/);
int ip_is_private(char *ip);
int ip_to_array(char* ip, uint8_t *arr);

#ifdef	__cplusplus
}
#endif

#endif	/* UTILS_H */
