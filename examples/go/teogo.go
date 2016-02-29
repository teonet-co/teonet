package main

/**
 * Teonet go application
 *
 * ### Build: 
    gcc -g -c -fPIC teogo_lib.c -I../../src -I../../embedded/libpbl/src -I../../embedded/teocli/libteol0
    ar cru teogo_lib.a teogo_lib.o   
    go build teogo.go
 *
 * ### See dependences: 
    readelf -d teogo
 *
 * ### Run: 
    ./teogo teo-go -a 172.18.0.30
 *
 */

/*
#cgo CFLAGS:  -I../../src -I../../embedded/libpbl/src -I../../embedded/teocli/libteol0 
#cgo LDFLAGS: -L../../src -lteonet teogo_lib.a

#include "ev_mgr.h"

void AC_event_cb_func(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, 
        size_t data_len, void *user_data);

*/
import "C"

import ("os"
        "fmt"
        "unsafe"
)

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

    // Initialize teonet event manager and Read configuration
    // (C.ksn_event_cb_type)(unsafe.Pointer())
    ke := C.ksnetEvMgrInit(C.int(len(os.Args)), argv, 
            C.ksn_event_cb_type(C.AC_event_cb_func), C.READ_ALL);

    // Set application type
    C.teoSetAppType(ke, C.CString("teo-go"));

    // Start teonet
    C.ksnetEvMgrRun(ke);

    // Free argv
    C.ksnet_stringArrFree((*C.ksnet_stringArr)(&argv));
}
