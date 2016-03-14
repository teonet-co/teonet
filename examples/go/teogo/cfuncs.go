
package main

/*

#include "ev_mgr.h"

extern void AGo_event_cb_func(ksnetEvMgrClass *ke, ksnetEvMgrEvents ev, void *data, size_t data_len, void *user_data);

// ksnetEvMgrInit wrapper
ksnetEvMgrClass *AC_ksnetEvMgrInit(int argc, char **argv) {

    // AGo_event_cb_func
    return ksnetEvMgrInit(argc, argv, AGo_event_cb_func, READ_ALL);
}
*/
import "C"

