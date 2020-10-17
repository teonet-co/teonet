/** 
 * \file   daemon.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Start stop daemon mode for application.
 *
 * Created on September 4, 2015, 11:20 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#include "daemon.h"
#include "pidfile.h"
#include "utils/utils.h"
#include "config/conf.h"
#include "config/config.h"

#ifndef HAVE_MINGW

/**
 * Kill started application in daemon mode
 *
 * @param argv Application argv argument
 * @param other_pid Process ID
 */
void kill_other (char **argv, int other_pid) {

    if (other_pid != 0) {

        // Send signal to main server process and all their children
        if (kill(-other_pid, SIGINT) != 0) {

            switch (errno) {

                case ESRCH:
                    fprintf(stderr, "Application is not running\n\n");
                    break;

                case EPERM:
                    fprintf(stderr, "Not enough privileges.\n"
                                    "Use: sudo %s ... -k\n\n", argv[0]);
                    break;

                default:
                    fprintf(stderr, "Unknown error\n\n");
            }
        }
        
        else {
            
            printf("Application is successfully stopped\n\n");
            syslog (LOG_NOTICE, "Successfully stopped [%d]\n", other_pid);
        }
    }
    else fprintf(stderr, "Application is not running\n\n");
}

/**
 * Start or stop daemon mode
 *
 * @param argv Application argv argument
 * @param conf Pointer to ksnet_cfg structure
 */
void start_stop_daemon(char **argv, ksnet_cfg *conf) {

    if(!(conf->dflag || conf->kflag)) return;

    // Check pid file name
    init_pidfile(conf->port);
    int port_pid = 0;
    read_pidfile(&port_pid);

    // Kill application in Daemon mode
    if(conf->kflag) {

        printf("Stopping application running at port %d...\n", 
                (int)conf->port);

        kill_other(argv, port_pid);
        remove_pidfile();
        kill_pidfile();
        closelog();

        exit(EXIT_SUCCESS);
    }

    // Start application as daemon
    if(conf->dflag) {

        // Check pip file exist
        if(!check_pid(port_pid)) {

            // Try to create pid file
            if(write_pidfile() != 0) {

                ksnet_printf(conf, ERROR_M,
                        "Can't start application in daemon mode: "
                        "no enough privileges\n"
                        "Use: sudo %s ... -d\n\n", argv[0]);

                exit(EXIT_FAILURE);
            }
            remove_pidfile();

            printf("Application started at port %d in daemon mode\n\n", 
                   (int)conf->port);

            // Start daemon mode
            if(daemon(FALSE, FALSE) == 0) {

                openlog(argv[0], LOG_NDELAY | LOG_PID, LOG_DAEMON);

                if(write_pidfile() != 0) {

                    ksnet_printf(conf, ERROR_M,
                            "Can't start application in daemon mode: "
                            "error during write the pid file\n");

                    syslog(LOG_ERR, "Error during write the pid file\n");

                    kill_pidfile();
                    closelog();
                    exit(EXIT_FAILURE);
                } 
                else syslog(LOG_NOTICE, "Started at port %d\n", (int)conf->port);           
            }

            // Can't start daemon mode
            else {

                ksnet_printf(conf, ERROR_M,
                          "Can't start application in daemon mode: "
                          "error during daemon starting\n");

                syslog(LOG_ERR, "Error during daemon starting\n");

                kill_pidfile();
                closelog();
                exit(EXIT_FAILURE);
            }
        }

        // Pid file already exist
        else {

            ksnet_printf(conf, ERROR_M,
              "Another one daemon is running. Use -k option to kill it.\n\n");

            kill_pidfile();
            closelog();
            exit(EXIT_FAILURE);
        }
    }
}

#endif
