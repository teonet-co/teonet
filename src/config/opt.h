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

enum ksnetEvMgrOpts {
    
    READ_OPTIONS = 01,
    READ_CONFIGURATION,
    READ_ALL = READ_OPTIONS|READ_CONFIGURATION,
    APP_PARAM
};

#ifdef	__cplusplus
extern "C" {
#endif

char ** ksnet_optRead(int argc, char **argv, ksnet_cfg *conf,
        int app_argc, char** app_argv, int show_opt);

void ksnet_optSetApp(ksnet_cfg *conf,
                     const char* app_name,
                     const char* app_prompt,
                     const char* app_description);

#ifdef	__cplusplus
}
#endif

#endif	/* OPT_H */

