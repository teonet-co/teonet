/*
 * File:   opt.h
 * Author: Kirill Scherba
 *
 * Created on June 6, 2014, 5:39 PM
 */

#ifndef OPT_H
#define	OPT_H

//#include "ev_mgr.h"
#include "conf.h"

#define SHOW_PEER_CONTINUOSLY 2

#define READ_OPTIONS 01
#define READ_CONFIGURATION 02

#ifdef	__cplusplus
extern "C" {
#endif

char ** ksnet_optRead(int argc, char **argv, ksnet_cfg *conf,
        int app_argc, char** app_argv, int show_opt);

#ifdef	__cplusplus
}
#endif

#endif	/* OPT_H */

