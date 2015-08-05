/* 
 * File:   rtudp_rheap.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet RT-UDP test
 *
 * Created on August 6, 2015, 12:13 AM
 */

#include <stdio.h>
#include <stdlib.h>

#include "ev_mgr.h"

// Test RT-UDP Receive Heap functions
void test1() {
    
    // Create Receive Heap
    PblHeap *receive_heap = pblHeapNew();
    pblHeapSetCompareFunction(receive_heap, ksnTRUDPReceiveHeapCompare);
    
    // Add some records to Receive Heap
    ksnTRUDPReceiveHeapAdd(receive_heap, 15, "Hello", 6);
    ksnTRUDPReceiveHeapAdd(receive_heap, 12, "Hello", 6);
    ksnTRUDPReceiveHeapAdd(receive_heap, 9, "Hello", 6);
    ksnTRUDPReceiveHeapAdd(receive_heap, 14, "Hello", 6);
    
    // Show Receive Heap and remove from Heap
    while(pblHeapSize(receive_heap)) {
        rh_data *rh_d = pblHeapGetFirst(receive_heap);
        printf("%2d %s\n", rh_d->id, (char*)rh_d->data);
        pblHeapRemoveFirst(receive_heap);
    }
    
    // Destroy Receive Heap
    pblHeapFree(receive_heap);
}

/**
 * Main application function
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char** argv) {

    
    printf("%%SUITE_STARTING%% teonet\n");
    printf("%%SUITE_STARTED%%\n");

    printf("%%TEST_STARTED%% test1 (Test RT-UDP Receive Heap functions)\n");
    test1();
    printf("%%TEST_FINISHED%% time=0 test1\n");
    
    printf("%%SUITE_FINISHED%% time=0\n");

    return (EXIT_SUCCESS);
}

