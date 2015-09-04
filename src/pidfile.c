/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * \file pidfile.c
 * \copyright Copyright (C) Kirill Scherba 2011-2015 <kirill@scherba.ru>
 *
 * \license
 * TEONET is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TEONET is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

//#include <ksnet/ksnet.h>

#include "pidfile.h"
#include "utils/utils.h"

#ifndef HAVE_MINGW

#if TRUE // RELEASE_KSNET
// Writing to this directory demands root privileges
#define PIDFILE_DIR "/var/run/"
#define GET_PIDFILE_DIR() strdup(PIDFILE_DIR)
#else
//#define PIDFILE_DIR ".ksnet/"
#define GET_PIDFILE_DIR()  ksnet_formatMessage("%s/", ksnet_getDataDir())
#endif

#define PIDFILE_EX "pid"
#define PIDFILE_NAME_LEN 128
#define EXE_NAME "ksnet"

// File for lock port
static char* pidfilename_port = NULL;

/**
 * Create the pid file name
 *
 * @param port
 */
void init_pidfile(int port) {

    if (port) {
        
        if (pidfilename_port != NULL) {

            free (pidfilename_port);
        }

        char *pid_file_dir = GET_PIDFILE_DIR();
        pidfilename_port = malloc(PIDFILE_NAME_LEN);
        sprintf(pidfilename_port, "%s%s_%d.%s", pid_file_dir, EXE_NAME, port,
            PIDFILE_EX);
        free(pid_file_dir);
    }
}

/**
 * Free the pid file name
 */
void kill_pidfile() {

    if (pidfilename_port != NULL) {

        free(pidfilename_port);
        pidfilename_port = NULL;
    }
}

/**
 * Write current pid to the specified pidfile
 *
 * @param pidfilename
 * @return
 */
int _write_pidfile (char* pidfilename) {

    if (pidfilename != NULL) {

        FILE* file = fopen(pidfilename, "w+");
        if (file != NULL) {

            if (fprintf(file, "%d", getpid()) < 0) {

                fclose(file);
                return -1;
            }
            else {

                fclose(file);
                return 0;
            }
        }
    }

    return -1;
}

/**
 * Write current pid to the all pidfiles
 *
 * @return
 */
int write_pidfile () {

    int result = 0;

    result += _write_pidfile (pidfilename_port);

    return result;
}

/**
 * Read current pid from the specified pidfile
 *
 * @param pidfilename
 * @return
 */
int _read_pidfile (char* pidfilename) {

    if (pidfilename != NULL) {

        FILE* file = fopen(pidfilename, "r");
        if (file != NULL) {

            int pid = 0;
            if (fscanf(file, "%d", &pid) != 1) {

                pid = 0;
            }

            fclose(file);
            return pid;
        }
    }

    return 0;
}

/**
 * Read current pid from the all pidfiles
 *
 * @param port_pid
 */
void read_pidfile (int* port_pid) {

    if (port_pid != NULL) {

        *port_pid = _read_pidfile (pidfilename_port);
    }
}

/**
 * Search for process with pid
 *
 * @param pid
 * @return
 */
int check_pid (int pid) {

    if (pid == 0)
        return FALSE;

    if (pid == getpid())
        return FALSE;

    if (kill(pid, 0) && errno == ESRCH)
        return FALSE;

    return TRUE;
}

/**
 * Remove the pid files
 */
void remove_pidfile () {

    if (pidfilename_port != NULL) {

        //if (_read_pidfile (pidfilename_port) == getpid())
        //{
        remove(pidfilename_port);
        //}
    }
}

#endif
