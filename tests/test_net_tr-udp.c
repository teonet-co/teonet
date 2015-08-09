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

extern CU_pSuite pSuite;

// Emulate initialization of ksnCoreClass 
#define kc_emul() \
  ksnetEvMgrClass ke; \
  ksnCoreClass kc; \
  kc.ke = &ke

/**
 * Test pblHeap functions
 */
void test_2_1() {
    
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
    CU_ASSERT(0 <= ksnTRUDPreceiveHeapAdd(NULL, receive_heap, 15, "Hello 15", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 <= ksnTRUDPreceiveHeapAdd(NULL, receive_heap, 12, "Hello 12", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 <= ksnTRUDPreceiveHeapAdd(NULL, receive_heap,  9, "Hello  9", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 <= ksnTRUDPreceiveHeapAdd(NULL, receive_heap, 14, "Hello 14", 9, (__SOCKADDR_ARG) &addr, addr_len));
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
 * Test Initialize/Destroy TR-UDP module
 */
void test_2_2() {
    
    // Emulate ksnCoreClass 
    kc_emul();
    
    ksnTRUDPClass *tu; // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL((tu = ksnTRUDPinit(&kc)));
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu->ip_map);
    ksnTRUDPDestroy(tu); // Destroy ksnTRUDPClass
    CU_PASS("Destroy ksnTRUDPClass done");
}

/**
 * Test TR-UDP utility functions
 */
void test_2_3() {

    // Emulate ksnCoreClass 
    kc_emul();
    
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
    // Initialize ksnTRUDPClass
    ksnTRUDPClass *tu = ksnTRUDPinit(&kc); 
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu);
       
    // 1) ksnTRUDPipMapData: Get IP map record by address or create new record if not exist
    ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, (__CONST_SOCKADDR_ARG) &addr,
            key, KSN_BUFFER_SM_SIZE);
    CU_ASSERT_STRING_EQUAL(key, tst_key); // Check key
    CU_ASSERT_PTR_NOT_NULL_FATAL(ip_map_d); // Check IP map created
    CU_ASSERT(ip_map_d->id == 0); // Check IP map created
    CU_ASSERT(ip_map_d->expected_id == 0); // Check IP map created
    CU_ASSERT_PTR_NOT_NULL_FATAL(ip_map_d->send_list); // Check send list created
    CU_ASSERT_PTR_NOT_NULL_FATAL(ip_map_d->receive_heap); // Check receive heap created
    CU_ASSERT(pblMapSize(ip_map_d->send_list) == 0); // Check send list functional
    CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 0); // Check receive heap functional

    // Destroy ksnTRUDPClass
    ksnTRUDPDestroy(tu); 
    CU_PASS("Destroy ksnTRUDPClass done");
    
     /* ---------------------------------------------------------------------- */
    // 2) ksnTRUDPKeyCreate: Create key from address
    key_len = ksnTRUDPkeyCreate(NULL, (__CONST_SOCKADDR_ARG) &addr, key, 
            KSN_BUFFER_SM_SIZE);
    // Check key
    CU_ASSERT(key_len == strlen(tst_key));
    CU_ASSERT_STRING_EQUAL_FATAL(key, tst_key);

    /* ---------------------------------------------------------------------- */
    // 3) ksnTRUDPKeyCreateAddr: Create key from string address and integer port
    key_len = ksnTRUDPkeyCreateAddr(NULL, addr_str, port, key, KSN_BUFFER_SM_SIZE);
    // Check key
    CU_ASSERT(key_len == strlen(tst_key));
    CU_ASSERT_STRING_EQUAL_FATAL(key, tst_key);
}

// Test RT-UDP reset functions
void test_2_4() {
    
    int i;
    for (i = 0; i < 2; i++) {
        
        // Emulate ksnCoreClass 
        kc_emul();

        // Test constants and variables
    //    const char *tst_key = "127.0.0.1:1327";
        const char *addr_str = "127.0.0.1"; 
    //    char key[KSN_BUFFER_SM_SIZE];
        struct sockaddr_in addr;
        const int port = 1327;
    //    size_t key_len;

        // Fill address 
        if(inet_aton(addr_str, &addr.sin_addr) == 0) CU_ASSERT(1 == 0);
        addr.sin_port = htons(port);

        // Initialize ksnTRUDPClass
        ksnTRUDPClass *tu = ksnTRUDPinit(&kc); // Initialize ksnTRUDPClass
        CU_ASSERT_PTR_NOT_NULL_FATAL(tu);

        // Add records to send list
        // Create and Get pointer to Send List ---------------------
        PblMap *sl = ksnTRUDPsendListGet(tu, (__CONST_SOCKADDR_ARG) &addr, NULL, 0); 
        CU_ASSERT_PTR_NOT_NULL_FATAL(sl);
        // Get 1 new ID = 0 ------------------
        uint32_t id = ksnTRUDPSendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr, sizeof(addr));
        CU_ASSERT(id == 0);
        // Add 1 message to Send List
        sl_data sl_d;
        sl_d.w = NULL;
        sl_d.data = (void*) "Some data 1";
        sl_d.data_len = 12;
        pblMapAdd(sl, &id, sizeof (id), (void*) &sl_d, sizeof (sl_d));
        CU_ASSERT(pblMapSize(sl) == 1);
        // Get 2 new ID = 1 ------------------
        id = ksnTRUDPSendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr, sizeof(addr));
        CU_ASSERT(id == 1);
        // Add 2 message to Send List
        sl_d.w = NULL;
        sl_d.data = (void*) "Some data 2";
        sl_d.data_len = 12;
        pblMapAdd(sl, &id, sizeof (id), (void*) &sl_d, sizeof (sl_d));
        CU_ASSERT(pblMapSize(sl) == 2);

        // Add records to receive heap
        // Add 1 records to receive heap
        ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, (__CONST_SOCKADDR_ARG) &addr, NULL, 0);
        ksnTRUDPreceiveHeapAdd(tu, ip_map_d->receive_heap, ip_map_d->expected_id, 
                "Some data 1", 12, (__CONST_SOCKADDR_ARG) &addr, sizeof(addr));
        CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 1);
        CU_ASSERT(ip_map_d->expected_id == 0);
        // Add 2 records to receive heap
        //ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, (__CONST_SOCKADDR_ARG) &addr, NULL, 0);
        ksnTRUDPreceiveHeapAdd(tu, ip_map_d->receive_heap, ip_map_d->expected_id, 
                "Some data 2", 12, (__CONST_SOCKADDR_ARG) &addr, sizeof(addr));
        CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 2);
        CU_ASSERT(ip_map_d->expected_id == 0);

        /* ---------------------------------------------------------------------- */
        // 1) ksnTRUDPreset: Remove send list and receive heap by input address
        ksnTRUDPreset(tu, (__CONST_SOCKADDR_ARG) &addr, i);
        if(i == 0) {
            CU_ASSERT(pblMapSize(tu->ip_map) == 1);
            CU_ASSERT(pblMapSize(sl) == 0);
            CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 0);
        } else {
            CU_ASSERT(pblMapSize(tu->ip_map) == 0); // All IP map records was removed 
        }

        // Destroy ksnTRUDPClass    
        ksnTRUDPDestroy(tu); 
        CU_PASS("Destroy ksnTRUDPClass done");    
    }
}

/**
 * Add TR-UDP suite tests
 * 
 * @return 
 */
int add_suite_2_tests(void) {
    
    // Add the tests to the suite 
    if ((NULL == CU_add_test(pSuite, "pblHeap functions", test_2_1)) ||
        (NULL == CU_add_test(pSuite, "Initialize/Destroy TR-UDP module", test_2_2)) ||
        (NULL == CU_add_test(pSuite, "TR-UDP utility functions", test_2_3)) ||
        (NULL == CU_add_test(pSuite, "RT-UDP reset functions", test_2_4))                
            ) {       
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    return 0;
}
