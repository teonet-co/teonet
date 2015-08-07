/*
 * File:   test_net_rt_udp.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * TR-UDP module test
 *
 * Created on Aug 7, 2015, 9:31:12 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "ev_mgr.h"
#include "net_tr-udp_.h"

/*
 * CUnit Test Suite
 */

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

/**
 * Test pblHeap functions
 */
void test1() {
    
    // Create Receive Heap
    rh_data *rh_d;
    PblHeap *receive_heap; 
    
    // Create new Heap and set compare function
    CU_ASSERT((receive_heap = pblHeapNew()) != NULL);
    pblHeapSetCompareFunction(receive_heap, ksnTRUDPReceiveHeapCompare);
    
    // Fill address
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    socklen_t addr_len = sizeof(addr);
        
    // Add some records to Receive Heap
    CU_ASSERT(pblHeapSize(receive_heap) == 0);
    CU_ASSERT(0 <= ksnTRUDPReceiveHeapAdd(NULL, receive_heap, 15, "Hello 15", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 <= ksnTRUDPReceiveHeapAdd(NULL, receive_heap, 12, "Hello 12", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 <= ksnTRUDPReceiveHeapAdd(NULL, receive_heap,  9, "Hello  9", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 <= ksnTRUDPReceiveHeapAdd(NULL, receive_heap, 14, "Hello 14", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(pblHeapSize(receive_heap) == 4);
    
    // Get saved Heap records and remove it
    // 9
    CU_ASSERT( (rh_d = pblHeapGetFirst(receive_heap)) != NULL);
    CU_ASSERT(rh_d->id == 9);
    pblHeapRemoveFirst(receive_heap);
    // 12
    CU_ASSERT( (rh_d = pblHeapGetFirst(receive_heap)) != NULL);
    CU_ASSERT(rh_d->id == 12);
    pblHeapRemoveFirst(receive_heap);
    // 14
    CU_ASSERT( (rh_d = pblHeapGetFirst(receive_heap)) != NULL);
    CU_ASSERT(rh_d->id == 14);
    pblHeapRemoveFirst(receive_heap);    
    // 15
    CU_ASSERT( (rh_d = pblHeapGetFirst(receive_heap)) != NULL);
    CU_ASSERT(rh_d->id == 15);
    pblHeapRemoveFirst(receive_heap);
    //
    CU_ASSERT(pblHeapSize(receive_heap) == 0);
            
    // Destroy Receive Heap
    pblHeapFree(receive_heap);
    
    CU_PASS("done");
}

/**
 * Initialize/Destroy TR-UDP module
 */
void test2() {
    
    ksnTRUDPClass *tu;
    CU_ASSERT((tu= ksnTRUDPInit(NULL)) != NULL);
    CU_ASSERT(tu->ip_map != NULL);
}

void test3() {
    //CU_ASSERT(1);
}

int main() {
    CU_pSuite pSuite = NULL;

    /* Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* Add a suite to the registry */
    pSuite = CU_add_suite("Teonet library TR-UDP module", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "pblHeap functions", test1)) ||
        (NULL == CU_add_test(pSuite, "Initialize/Destroy TR-UDP module", test2)) ||
        (NULL == CU_add_test(pSuite, "test3", test3))
            ) {       
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
