%module teonet
%{
#include "../../../src/ev_mgr.h"
    
//extern const char *teoGetLibteonetVersion();
//extern ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb /*NULL*/, READ_ALL);
%}
/* If we don't want to access teoGetLibteonetVersion function from the perl, then comment
the below line.
*/
extern const char *teoGetLibteonetVersion();
extern ksnetEvMgrClass *ksnetEvMgrInit(
    int argc, char** argv,
    ksn_event_cb_type event_cb,
    int options
);
extern int ksnetEvMgrRun(ksnetEvMgrClass *ke);
