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

/**
 * Convert integer to string
 *
 * @param ival
 * @return
 */
char* itoa(int ival) {

    char buffer[KSN_BUFFER_SIZE];
    snprintf(buffer, KSN_BUFFER_SIZE, "%d", ival);

    return strdup(buffer);
}

CU_pSuite pSuite = NULL;

/*
 * CUnit Test Suite
 */

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
    CU_PASS("pblHeapFree done");
}

/**
 * Initialize/Destroy TR-UDP module
 */
void test2() {
    
    ksnTRUDPClass *tu;
    CU_ASSERT_PTR_NOT_NULL_FATAL((tu = ksnTRUDPInit(NULL)));
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu->ip_map);
    //ksnTRUDPDestroy(tu);
}

void test3() {

    // Test constants and variables
    const char *tst_key = "127.0.0.1:1327";
    const char *addr_str = "127.0.0.1"; 
    char key[KSN_BUFFER_SM_SIZE];
    struct sockaddr_in addr;
    const int port = 1327;
    size_t key_len;

    // Fill address 
    if(inet_aton(addr_str, &addr.sin_addr) == 0) CU_ASSERT(1 == 0);
    addr.sin_port = htons(port);

    /* ---------------------------------------------------------------------- */
    ksnTRUDPClass *tu = ksnTRUDPInit(NULL); // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu);
   
    // 1) ksnTRUDPIpMapData: Get IP map record by address or create new record if not exist
    ip_map_data *ip_map_d = ksnTRUDPIpMapData(tu, (__CONST_SOCKADDR_ARG) &addr,
            key, KSN_BUFFER_SM_SIZE);
    CU_ASSERT_STRING_EQUAL(key, tst_key); // Check key
    CU_ASSERT_PTR_NOT_NULL_FATAL(ip_map_d); // Check IP map created
    CU_ASSERT(ip_map_d->id == 0); // Check IP map created
    CU_ASSERT(ip_map_d->expected_id == 0); // Check IP map created
    CU_ASSERT_PTR_NOT_NULL_FATAL(ip_map_d->send_list); // Check send list created
    CU_ASSERT_PTR_NOT_NULL_FATAL(ip_map_d->receive_heap); // Check receive heap created
    CU_ASSERT(pblMapSize(ip_map_d->send_list) == 0); // Check send list functional
    CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 0); // Check receive heap functional
    
    //ksnTRUDPDestroy(tu); // Destroy ksnTRUDPClass

     /* ---------------------------------------------------------------------- */
    // 2) ksnTRUDPKeyCreate: Create key from address
    key_len = ksnTRUDPKeyCreate(NULL, (__CONST_SOCKADDR_ARG) &addr, key, 
            KSN_BUFFER_SM_SIZE);
    // Check key
    CU_ASSERT(key_len == strlen(tst_key));
    CU_ASSERT_STRING_EQUAL_FATAL(key, tst_key);

    /* ---------------------------------------------------------------------- */
    // 3) ksnTRUDPKeyCreateAddr: Create key from string address and integer port
    key_len = ksnTRUDPKeyCreateAddr(NULL, addr_str, port, key, KSN_BUFFER_SM_SIZE);
    // Check key
    CU_ASSERT(key_len == strlen(tst_key));
    CU_ASSERT_STRING_EQUAL_FATAL(key, tst_key);
}

int init_suite(void) {    
    return 0;
}

int add_tests(void) {
    
    /* Add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "pblHeap functions", test1)) ||
        (NULL == CU_add_test(pSuite, "Initialize/Destroy TR-UDP module", test2)) ||
        (NULL == CU_add_test(pSuite, "TR-UDP utility functions", test3))
            ) {       
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    return 0;
}

int main() {
    
    KSN_SET_TEST_MODE(1);
            
    /* Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* Add a suite to the registry */
    pSuite = CU_add_suite("Teonet library TR-UDP module", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    add_tests();

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    //CU_list_tests_to_file();
    CU_basic_run_tests();
    //CU_console_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
