/** 
 * \file   daemon.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Start stop daemon mode for application.
 *
 * Created on September 4, 2015, 11:20 PM
 */

#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>

#include "daemon.h"
#include "config/config.h"

#ifndef HAVE_MINGW

/**
 * Printing message to syslog
 * 
 * This function like perror but it is printing message to syslog
 *
 * @param prefix Log message prefix
 */
void lperror(char* prefix)
{
    if (prefix != NULL)
        syslog(LOG_ERR, "%s: %d - %s\n", prefix, errno, strerror (errno));
    else
        syslog(LOG_ERR, "%d - %s\n", errno, strerror (errno));
}

#endif
