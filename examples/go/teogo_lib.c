/** 
 * \file   teogo_lib.c
 * \author Kirill Scherba <kirill@scherba.ru> 
 *
 * Created on February 29, 2016, 1:13 AM
 */

#include "ev_mgr.h"

/*
 
### Build:
 
    gcc -g -c -fPIC teogo_lib.c -I../../src -I../../embedded/libpbl/src -I../../embedded/teocli/libteol0
    ar cru teogo_lib.a teogo_lib.o   
 
 */

extern void AGo_event_cb_func(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, 
        void *data, size_t data_len, void *user_data);

void AC_event_cb_func(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data) {
    
    AGo_event_cb_func(ke, event, data, data_len, user_data);
}
