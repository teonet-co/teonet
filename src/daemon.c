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

void lperror(char* prefix);

/**
 * Kill other server
 *
 * @param other_pid
 */
void kill_other (int other_pid) {

    if (other_pid != 0) {

        // Send signal to main server process and all their children
        if (kill(-other_pid, SIGUSR1) != 0) {

            switch (errno) {

                case ESRCH:
                    fprintf(stderr, "Application is not running\n\n");
                    break;

                case EPERM:
                    fprintf(stderr, "Not enough privileges.\n"
                                    "Use: sudo ksnet ... -k\n\n");
                    break;

                default:
                    fprintf(stderr, "Unknown error\n\n");
            }
        }
        else
        {
            ksnet_printf(MESSAGE, "Application is successfully stopped\n\n");
            syslog (LOG_NOTICE, "Successfully stopped [%d]\n", other_pid);

        }
    }
    else
    {
        fprintf(stderr, "Application is not running\n\n");
    }
}

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
