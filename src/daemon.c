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

void lperror(char* prefix);

/**
 * Kill started application in daemon mode
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
            printf("Application is successfully stopped\n\n");
            syslog (LOG_NOTICE, "Successfully stopped [%d]\n", other_pid);
        }
    }
    else
    {
        fprintf(stderr, "Application is not running\n\n");
    }
}

/**
 * Initial signals handlers
 */
void init_signals() {

//    struct sigaction sa;
}

/**
 * Before application is stopped
 * @param s
 */
void stopped_handler(int s) {

    signal(s,SIG_IGN);
    //ksnet_disconnect();
// \todo    ksnetEvMgrStop(ke);
    printf("\rApplication has been stopped by signal...\n");
//    int i = 110;
// \todo    while(!ksnet_isStoped() && --i) usleep(10000); // \todo: The ksnet_isStoped never return true !!!
    printf("Bye!");
}

/**
 * Start or stop daemon mode
 *
 * @param argv Application argv argument
 * @param conf Pointer to ksnet_cfg structure
 */
void start_stop_daemon(char **argv, ksnet_cfg *conf) {

    // Initial signals
    init_signals();

    if(!(conf->dflag || conf->kflag)) return;

    // Check pid file name
    init_pidfile(conf->port);
    int port_pid = 0;
    read_pidfile(&port_pid);

    // Kill application in Daemon mode
    if(conf->kflag) {

        printf("Stopping application running at port %d...\n", (int)conf->port);

        kill_other(port_pid);
        remove_pidfile();
        kill_pidfile();
        closelog();

        exit(EXIT_SUCCESS);
    }

    // Start as daemon
    if(conf->dflag) {

      if(!check_pid(port_pid)) {

          // Try to create pid file
//          #if RELEASE_KSNET
          if(write_pidfile() != 0) {

            ksnet_printf(conf, ERROR_M,
                    "Can't start application in daemon mode: "
                    "no enough privileges\n"
                    "Use: sudo %s ... -d\n\n", argv[0]);

            exit(EXIT_SUCCESS);
          }
          remove_pidfile();
//          #endif

          printf("Application started at port %d in daemon mode\n\n", 
                  (int)conf->port);

//          #if RELEASE_KSNET
          if(daemon(FALSE, FALSE) == 0)
//          #else
//          if(daemon(TRUE, FALSE) == 0)
//          #endif
          {
            openlog(argv[0], LOG_NDELAY | LOG_PID, LOG_DAEMON);

            if(write_pidfile() != 0) {

              syslog(LOG_ERR,"Error during write the pid file\n");
              kill_pidfile();
              closelog();
              exit(EXIT_SUCCESS);
            }

            else {

              syslog(LOG_NOTICE, "Started at port %d\n", (int)conf->port);
            }
          }

          else {

              fprintf(stderr, "Error during daemon starting\n");
              kill_pidfile();
              closelog();
              exit(EXIT_SUCCESS);
          }
      }

      else {

          ksnet_printf(conf, MESSAGE,
            "Another one daemon is running. Use -k option to kill it.\n\n");

          kill_pidfile();
          closelog();
          exit(EXIT_SUCCESS);
      }
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
