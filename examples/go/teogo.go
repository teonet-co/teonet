package main

// # Build: go build -gccgoflags="-lteonet" teogo.go

/*
#cgo CFLAGS:  -I../../src -I../../embedded/libpbl/src -I../../embedded/teocli/libteol0
#cgo LDFLAGS: -L../../src -lteonet -lcrypto
#include "ev_mgr.h"
*/
import "C"
import "os"
//import "fmt"

func main() {

    // Initialize teonet event manager and Read configuration
    ke := C.ksnetEvMgrInit(C.int(len(os.Args)), (*C.CString)(os.Args), /*event_cb*/ nil, C.READ_ALL);
    //fmt.Print("%d", ke.argc);

    // Set application type
    C.teoSetAppType(ke, C.CString("teo-go"));

    // Start teonet
    C.ksnetEvMgrRun(ke);
}
