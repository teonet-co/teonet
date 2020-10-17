/** 
 * \file   daemon.h
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Created on September 4, 2015, 11:21 PM
 */

#ifndef DAEMON_H
#define	DAEMON_H

#include "config/conf.h"

#ifdef	__cplusplus
extern "C" {
#endif

void start_stop_daemon(char **argv, ksnet_cfg *conf);

#ifdef	__cplusplus
}
#endif

#endif	/* DAEMON_H */
