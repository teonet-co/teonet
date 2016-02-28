package main

/**
 * Teonet go application
 *
 * ### Build: 
 *    go build -gccgoflags="-lteonet" teogo.go
 *
 * ### See dependences: 
 *    readelf -d teogo
 *
 * ### Run: 
 *    ./teogo teo-go -a 172.18.0.30
 *
 */

/*
#cgo CFLAGS:  -I../../src -I../../embedded/libpbl/src -I../../embedded/teocli/libteol0
#cgo LDFLAGS: -L../../src -lteonet
//cgo pkg-config: teonet
#include "ev_mgr.h"
*/
import "C"
import "os"

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

func main() {

    // Get argv
    argv := argsToargv(os.Args);

    // Initialize teonet event manager and Read configuration
    ke := C.ksnetEvMgrInit(C.int(len(os.Args)), argv, /*event_cb*/ nil, C.READ_ALL);

    // Set application type
    C.teoSetAppType(ke, C.CString("teo-go,teo-vpn"));

    // Start teonet
    C.ksnetEvMgrRun(ke);

    // Free argv
    C.ksnet_stringArrFree((*C.ksnet_stringArr)(&argv));
}
