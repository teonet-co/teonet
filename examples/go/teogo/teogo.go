package main

/**
 * Teonet go application
 *
 * ### Build: 
    #gcc -g -c -fPIC teogo_lib.c -I../../../src -I../../../embedded/libpbl/src -I../../../embedded/teocli/libteol0
    #ar cru teogo_lib.a teogo_lib.o   
    go build -gccgoflags="-lteonet" 
 
 * ### See dependences: 
    readelf -d teogo
 *
 * ### Run: 
    ./teogo teo-go -a 10.15.56.61
 *
 */


/*
#cgo CFLAGS:  -I../../../src -I../../../embedded/libpbl/src -I../../../embedded/teocli/libteol0 
#cgo LDFLAGS: -L../../../src -lteonet 

#include "ev_mgr.h"

ksnetEvMgrClass *AC_ksnetEvMgrInit(int argc, char **argv);
*/
import "C"

import ("os"
        "fmt"
        "unsafe"
)

//export AGo_event_cb_func
func AGo_event_cb_func(ke *C.ksnetEvMgrClass, ev C.ksnetEvMgrEvents, 
    data unsafe.Pointer, data_len C.size_t, user_data unsafe.Pointer) {
    
    switch ev {
        
        // EV_K_STARTED #0 Calls immediately after event manager starts
        case C.EV_K_STARTED: 
            
            fmt.Print("Teogo started .... \n");

        // Calls when new peer connected to host event
        case C.EV_K_CONNECTED:
            
            rd := (*C.ksnCorePacketData)(data);
            fmt.Printf("peer \"%s\" was connected\n", C.GoString(rd.from));
//            C.ksn_printf(ke, nil, C.DEBUG,
//                        "peer \"%s\" was connected\n", peer);

        // Calls when a peer disconnected \todo Check L0 and MM peers disconnection
        case C.EV_K_DISCONNECTED:
        
            rd := (*C.ksnCorePacketData)(data);
            fmt.Printf("peer \"%s\" was disconnected\n", C.GoString(rd.from));
//            C.ksn_printf(ke, nil, C.DEBUG,
//                        "peer \"%s\" was connected\n", peer);
    }    
}

/**
 * Main application function
 */
func main() {

    // Get argv
    argv := argsToargv(os.Args);

    // Initialize teonet event manager, Read configuration and connect to 
    // AGo_event_cb_func callback function
    ke := C.AC_ksnetEvMgrInit(C.int(len(os.Args)), argv);

    // Set application type
    C.teoSetAppType(ke, C.CString("teo-go"));
    C.teoSetAppVersion(ke, C.CString("0.0.1"));

    // Start teonet
    C.ksnetEvMgrRun(ke);

    // Free argv
    C.ksnet_stringArrFree((*C.ksnet_stringArr)(&argv));
}

/**
 * Convert []string to **C.char
 */
func argsToargv(args []string) (**C.char) {

    argv := C.ksnet_stringArrCreate();
    for i := 0; i < len(args); i++ {
	C.ksnet_stringArrAdd(&argv, C.CString(args[i]) );
    }

    return argv;
}
