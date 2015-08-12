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
  kc.ke = &ke; \
  ke.ev_loop = ev_loop_new (0)

/**
 * Test pblHeap functions
 */
void test_2_1() {

    // Create Receive Heap
    rh_data *rh_d;
    PblHeap *receive_heap;

    // Create new Heap and set compare function
    CU_ASSERT((receive_heap = pblHeapNew()) != NULL);
    pblHeapSetCompareFunction(receive_heap, ksnTRUDPreceiveHeapCompare);

    // Fill address
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    socklen_t addr_len = sizeof(addr);

    // Add some records to Receive Heap
    CU_ASSERT(pblHeapSize(receive_heap) == 0);
    CU_ASSERT(0 < ksnTRUDPreceiveHeapAdd(NULL, receive_heap, 15, "Hello 15", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 < ksnTRUDPreceiveHeapAdd(NULL, receive_heap, 12, "Hello 12", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 < ksnTRUDPreceiveHeapAdd(NULL, receive_heap,  9, "Hello  9", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 < ksnTRUDPreceiveHeapAdd(NULL, receive_heap, 14, "Hello 14", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(pblHeapSize(receive_heap) == 4);

    // Get saved Heap records and remove it
    // 9
    CU_ASSERT( (rh_d = pblHeapGetFirst(receive_heap)) != NULL);
    CU_ASSERT(rh_d->id == 9);
    CU_ASSERT_STRING_EQUAL(rh_d->data, "Hello  9");
    pblHeapRemoveFirst(receive_heap);
    // 12
    CU_ASSERT( (rh_d = pblHeapGetFirst(receive_heap)) != NULL);
    CU_ASSERT(rh_d->id == 12);
    CU_ASSERT_STRING_EQUAL(rh_d->data, "Hello 12");
    pblHeapRemoveFirst(receive_heap);
    // 14
    CU_ASSERT( (rh_d = pblHeapGetFirst(receive_heap)) != NULL);
    CU_ASSERT(rh_d->id == 14);
    CU_ASSERT_STRING_EQUAL(rh_d->data, "Hello 14");
    pblHeapRemoveFirst(receive_heap);
    // 15
    CU_ASSERT( (rh_d = pblHeapGetFirst(receive_heap)) != NULL);
    CU_ASSERT(rh_d->id == 15);
    CU_ASSERT_STRING_EQUAL(rh_d->data, "Hello 15");
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
    CU_ASSERT_STRING_EQUAL(key, tst_key);

    /* ---------------------------------------------------------------------- */
    // 3) ksnTRUDPKeyCreateAddr: Create key from string address and integer port
    key_len = ksnTRUDPkeyCreateAddr(NULL, addr_str, port, key, KSN_BUFFER_SM_SIZE);
    // Check key
    CU_ASSERT(key_len == strlen(tst_key));
    CU_ASSERT_STRING_EQUAL(key, tst_key);
}

// Test RT-UDP reset functions
void test_2_4() {

    int i;
    for (i = 0; i < 2; i++) {

        // Emulate ksnCoreClass
        kc_emul();

        // Test constants and variables
        const char *addr_str = "127.0.0.1";
        struct sockaddr_in addr;
        const int port = 1327;

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
        uint32_t id = ksnTRUDPsendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr);
        CU_ASSERT(id == 0);
        // Add 1 message to Send List
        sl_data sl_d;
//        sl_d.w = NULL;
        sl_d.data = (void*) "Some data 1";
        sl_d.data_len = 12;
        pblMapAdd(sl, &id, sizeof (id), (void*) &sl_d, sizeof (sl_d));
        CU_ASSERT(pblMapSize(sl) == 1);
        // Get 2 new ID = 1 ------------------
        id = ksnTRUDPsendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr);
        CU_ASSERT(id == 1);
        // Add 2 message to Send List
//        sl_d.w = NULL;
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
        if(i == 0) {
            ksnTRUDPreset(tu, (__CONST_SOCKADDR_ARG) &addr, i);
            CU_ASSERT(pblMapSize(tu->ip_map) == 1);
            CU_ASSERT(pblMapSize(sl) == 0);
            CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 0);
        }
        // 2) ksnTRUDPresetAddr: Remove send list and receive heap by input address
        else {
            ksnTRUDPresetAddr(tu, addr_str, port, i);
            CU_ASSERT(pblMapSize(tu->ip_map) == 0); // All IP map records was removed
        }

        // Destroy ksnTRUDPClass
        ksnTRUDPDestroy(tu);
        CU_PASS("Destroy ksnTRUDPClass done");

        // TODO: ksnTRUDPresetSend: Send reset to peer
    }
}

// Test RT-UDP send list functions
void test_2_5() {

    // Emulate ksnCoreClass
    kc_emul();

    // Test constants and variables
    const char *addr_str = "127.0.0.1";
    struct sockaddr_in addr;
    const int port = 1327;

    // Fill address
    if(inet_aton(addr_str, &addr.sin_addr) == 0) CU_ASSERT(1 == 0);
    addr.sin_port = htons(port);

    // Initialize ksnTRUDPClass
    ksnTRUDPClass *tu = ksnTRUDPinit(&kc); // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu);

    // 1) ksnTRUDPsendListGet: Create and Get pointer to Send List
    PblMap *sl = ksnTRUDPsendListGet(tu, (__CONST_SOCKADDR_ARG) &addr, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl);
    CU_ASSERT(pblMapSize(sl) == 0);

    // 2) ksnTRUDPsendListNewID: Get new ID
    uint32_t id = ksnTRUDPsendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr);
    CU_ASSERT(id == 0);

    // Add 1 message to send list
    sl_data sl_d;
    sl_d.w.data = NULL;
    sl_d.data = (void*) "Some data 1";
    sl_d.data_len = 12;
    pblMapAdd(sl, &id, sizeof (id), (void*) &sl_d, sizeof (sl_d));
    CU_ASSERT(pblMapSize(sl) == 1);

    // Add 2 message to send list
    id = ksnTRUDPsendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr);
    CU_ASSERT(id == 1);
    sl_d.w.data = NULL;
    sl_d.data = (void*) "Some data 2";
    sl_d.data_len = 12;
    pblMapAdd(sl, &id, sizeof (id), (void*) &sl_d, sizeof (sl_d));
    CU_ASSERT(pblMapSize(sl) == 2);

    // Add 3 message to send list
    id = ksnTRUDPsendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr);
    CU_ASSERT(id == 2);
    sl_d.w.data = NULL;
    sl_d.data = (void*) "Some data 3";
    sl_d.data_len = 12;
    pblMapAdd(sl, &id, sizeof (id), (void*) &sl_d, sizeof (sl_d));
    CU_ASSERT(pblMapSize(sl) == 3);

    // 3) ksnTRUDPSendListGetData: Get Send List timer watcher and stop it
    sl_data *sl_d_get = ksnTRUDPsendListGetData(tu, 1, (__CONST_SOCKADDR_ARG) &addr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl_d_get);
    CU_ASSERT_STRING_EQUAL(sl_d_get->data, "Some data 2");

    // 4) ksnTRUDPsendListRemove: Remove record from send list
    ksnTRUDPsendListRemove(tu, 0, (__CONST_SOCKADDR_ARG) &addr);
    CU_ASSERT(pblMapSize(sl) == 2);

    // 5) ksnTRUDPsendListRemove: Remove all record from send list
    ksnTRUDPsendListRemoveAll(tu, sl);
    CU_ASSERT(pblMapSize(sl) == 0);

    // 6 ksnTRUDPsendListAdd: Add packet to Sent message list
    id = ksnTRUDPsendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr);
    ksnTRUDPsendListAdd(tu, id, 0, 0, "Some data 4", 12, 0, 0, (__CONST_SOCKADDR_ARG) &addr, sizeof(addr));
    sl_d_get = ksnTRUDPsendListGetData(tu, id, (__CONST_SOCKADDR_ARG) &addr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl_d_get);
    CU_ASSERT_STRING_EQUAL(sl_d_get->data, "Some data 4");
    sl_timer_stop(ke.ev_loop, &sl_d_get->w);
    CU_ASSERT_PTR_NULL(sl_d_get->w.data);
    CU_PASS("Send list timer was stopped");
    
    // 7 ksnTRUDPSendListDestroyAll: Free all elements and free all Sent message lists
    ksnTRUDPsendListDestroyAll(tu);
    CU_PASS("Destroy all sent message lists done");

    // Destroy ksnTRUDPClass
    ksnTRUDPDestroy(tu);
    CU_PASS("Destroy ksnTRUDPClass done");
}

// Callback, this time for a time-out
static void timeout_cb (EV_P_ ev_timer *w, int revents) {

    //puts ("timeout");
    // this causes the innermost ev_run to stop iterating
    ev_break (EV_A_ EVBREAK_ONE);
}

// Test RT-UDP send list timer functions
void test_2_6() {

    // Emulate ksnCoreClass
    kc_emul();

    // Test constants and variables
    const char *addr_str = "127.0.0.1";
    struct sockaddr_in addr;
    const int port = 1327;

    // Fill address
    if(inet_aton(addr_str, &addr.sin_addr) == 0) CU_ASSERT(1 == 0);
    addr.sin_port = htons(port);

    // Initialize ksnTRUDPClass
    ksnTRUDPClass *tu = ksnTRUDPinit(&kc); // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu);

    // 1) sl_timer_start: Start the send list timer
    // Create and Get pointer to Send List
    PblMap *sl = ksnTRUDPsendListGet(tu, (__CONST_SOCKADDR_ARG) &addr, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl);
    // Get new ID
    uint32_t id = ksnTRUDPsendListNewID(tu, (__CONST_SOCKADDR_ARG) &addr);
    CU_ASSERT(id == 0);
    // Add message to send list and start the send list timer
    ksnTRUDPsendListAdd(tu, id, 0, CMD_TUN, "Some data 1", 12, 0, 0, (__CONST_SOCKADDR_ARG) &addr, sizeof(addr));
    CU_ASSERT(pblMapSize(sl) == 1); // Number of records in send list
    
    // 2) sl_timer_cb: Process send list timer callback       
    // Define timer to stop event loop after MAX_ACK_WAIT * 2 + 20%. This time need to run timer twice.
    ev_timer timeout_watcher;
    ev_timer_init (&timeout_watcher, timeout_cb, MAX_ACK_WAIT*2 + MAX_ACK_WAIT * 0.2, 0.0);
    ev_timer_start (ke.ev_loop, &timeout_watcher);     
    // Start event loop
    ev_run (ke.ev_loop, 0);   
    CU_ASSERT(pblMapSize(sl) == 1); // Number of records in send list
    printf(" ev_loop exit ...");
    // Check data in send list
    sl_data *sl_d_get = ksnTRUDPsendListGetData(tu, id, (__CONST_SOCKADDR_ARG) &addr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl_d_get);
    CU_ASSERT(sl_d_get->attempt == 2); // Number of attempt was done is equal 2

    // 3) sl_timer_stop: Stop the send list timer
    sl_timer_stop(ke.ev_loop, &sl_d_get->w);
    CU_ASSERT_PTR_NULL(sl_d_get->w.data);
    sl_timer_stop(ke.ev_loop, &sl_d_get->w); // Test stop timer twice
    CU_PASS("Send list timer was stopped");

    // Destroy ksnTRUDPClass
    ksnTRUDPDestroy(tu);
    CU_PASS("Destroy ksnTRUDPClass done");
}

// Test RT-UDP receive heap functions
void test_2_7() {
    
    // Emulate ksnCoreClass
    kc_emul();

    // Test constants and variables
    const size_t addr_len = sizeof(struct sockaddr_in);
    const char *addr_str = "127.0.0.1";
    struct sockaddr_in addr;
    const int port = 1327;

    // Fill address
    if(inet_aton(addr_str, &addr.sin_addr) == 0) CU_ASSERT(1 == 0);
    addr.sin_port = htons(port);

    // Initialize ksnTRUDPClass
    ksnTRUDPClass *tu = ksnTRUDPinit(&kc); // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu);

    // Get IP Map
    ip_map_data *ip_map_d = ksnTRUDPipMapData(tu, (__CONST_SOCKADDR_ARG) &addr, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ip_map_d);
    
    // ksnTRUDPReceiveHeapCompare
    
    // 1) ksnTRUDPreceiveHeapAdd, ksnTRUDPreceiveHeapCompare: Add record to the Receive Heap
    CU_ASSERT(0 < ksnTRUDPreceiveHeapAdd(tu, ip_map_d->receive_heap, 15, "Hello 15", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 < ksnTRUDPreceiveHeapAdd(tu, ip_map_d->receive_heap, 12, "Hello 12", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 < ksnTRUDPreceiveHeapAdd(tu, ip_map_d->receive_heap,  9, "Hello  9", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(0 < ksnTRUDPreceiveHeapAdd(tu, ip_map_d->receive_heap, 14, "Hello 14", 9, (__SOCKADDR_ARG) &addr, addr_len));
    CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 4);
    
    // 2) ksnTRUDPreceiveHeapGetFirst: Get first element of Receive Heap (with lowest ID) 
    rh_data *rh_d = ksnTRUDPreceiveHeapGetFirst(ip_map_d->receive_heap);
    CU_ASSERT_PTR_NOT_NULL_FATAL(rh_d);
    CU_ASSERT_STRING_EQUAL(rh_d->data, "Hello  9");
    CU_ASSERT(rh_d->id == 9);
    
    // 3) ksnTRUDPreceiveHeapRemoveFirst, ksnTRUDPreceiveHeapElementFree: Remove first element from Receive Heap (with lowest ID)
    CU_ASSERT(ksnTRUDPreceiveHeapRemoveFirst(tu, ip_map_d->receive_heap) == 1);
    CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 3);
    
    // 4) ksnTRUDPReceiveHeapRemoveAll: Remove all elements from Receive Heap
    ksnTRUDPreceiveHeapRemoveAll(tu, ip_map_d->receive_heap);
    CU_ASSERT(pblHeapSize(ip_map_d->receive_heap) == 0);
    
    // 5) ksnTRUDPReceiveHeapDestroyAll
    ksnTRUDPreceiveHeapDestroyAll(tu);
    CU_PASS("Destroy all receive heap");
    
    // Destroy ksnTRUDPClass
    ksnTRUDPDestroy(tu);
    CU_PASS("Destroy ksnTRUDPClass done");
}

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * Create and bind UDP socket for client/server
 *
 * @param kc
 * @return
 */
int bind_udp(int *port) {

    int fd, i;
    struct sockaddr_in addr;	// Our address 

    // Create a UDP socket
    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror(" cannot create socket ...");
        return -1;
    }

    // Bind the socket to any valid IP address and a specific port, increment 
    // port if busy 
    for(i=0;;) {
        
        memset((char *)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(*port);

        if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {

            printf(" try port %d ...", (*port)++);
            perror("  bind failed ...");
            if(i++ < NUMBER_TRY_PORTS) continue;
            else return -2;
        }
        else break;
    }

    //printf(" start UDP at %d, fd = %d ...", *port, fd);

    return fd;
}

// Test main RT-UDP function ksnTRUDPsendto
void test_2_8() {
    
    // Emulate ksnCoreClass
    kc_emul();

    // Test constants and variables
    const size_t addr_len = sizeof(struct sockaddr_in);
    const char *addr_str = "127.0.0.1";
    struct sockaddr_in addr_s, addr_r;
    int fd_s, fd_r, port_s = 9027, port_r = 9029;

    // Start test UDP sender
    fd_s = bind_udp(&port_s);
    CU_ASSERT(fd_s > 0);
    
    // Start test UDP receiver
    fd_r = bind_udp(&port_r);
    CU_ASSERT(fd_r > 0);
    
    // Fill sender address 
    if(inet_aton(addr_str, &addr_s.sin_addr) == 0) CU_ASSERT(1 == 0);
    addr_s.sin_family = AF_INET;
    addr_s.sin_port = htons(port_s);
    
    // Fill receiver address 
    if(inet_aton(addr_str, &addr_r.sin_addr) == 0) CU_ASSERT(1 == 0);
    addr_r.sin_family = AF_INET;
    addr_r.sin_port = htons(port_r);
    
    // Initialize ksnTRUDPClass
    ksnTRUDPClass *tu = ksnTRUDPinit(&kc); // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu);
    
    // 1) ksnTRUDPsendto test
    //
    // Prepare data to SendTo receiver
    const char *buf = "Hello world - 1"; // Data to send
    size_t buf_len = strlen(buf) + 1; // Size of send data
    const size_t tru_ptr = sizeof (ksnTRUDP_header); // TR-UDP header size
    //
    // a) Send one message receiver
    ssize_t sent = ksnTRUDPsendto(tu, 0, 0, 0, CMD_TUN, fd_s, buf, buf_len, 0, 
        (__CONST_SOCKADDR_ARG) &addr_r, addr_len); // TR-UDP sendto
    if(sent < 0) printf(" sent to %d = %d: %s ...", port_r, (int)sent, strerror(errno));
    // Check sent result
    CU_ASSERT_FATAL(sent > 0);
    CU_ASSERT(sent == buf_len + tru_ptr);
    //
    // Prepare data to Read packet from socket
    struct sockaddr_in addr_recv;
    char buf_recv[KSN_BUFFER_SIZE];
    size_t addr_recv_len = sizeof(addr_recv);
    ksnTRUDP_header *tru_header = (ksnTRUDP_header *) buf_recv;
    //
    // b) Receive packet from socket
    ssize_t recvlen = recvfrom(fd_r, buf_recv, KSN_BUFFER_SIZE, 0, (__CONST_SOCKADDR_ARG) &addr_recv, &addr_recv_len);
    if(recvlen < 0) printf(" recv = %d: %s ...", (int)recvlen, strerror(errno));
    // Check receive result
    CU_ASSERT_FATAL(recvlen == sent); // Receive length equal send length
    CU_ASSERT(tru_header->id == 0); // Packet id = 0
    CU_ASSERT(tru_header->payload_length == buf_len); // Payload length equal send buffer length
    CU_ASSERT_STRING_EQUAL(buf, buf_recv + tru_ptr); // Content of send buffer equal to received data
    //
    // c) Send next packet
    sent = ksnTRUDPsendto(tu, 0, 0, 0, CMD_TUN, fd_s, buf, buf_len, 0, 
        (__CONST_SOCKADDR_ARG) &addr_r, addr_len); // TR-UDP sendto
    CU_ASSERT_FATAL(sent > 0);
    //
    // d) Receive packet with next id = 1
    recvlen = recvfrom(fd_r, buf_recv, KSN_BUFFER_SIZE, 0, (__CONST_SOCKADDR_ARG) &addr_recv, &addr_recv_len);
    CU_ASSERT(tru_header->id == 1); // Packet id = 1
    //
    // e) Check send list - it should contain 2 records (with id = 0 and 1)
    PblMap *sl = ksnTRUDPsendListGet(tu, (__CONST_SOCKADDR_ARG) &addr_r, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl);
    CU_ASSERT(pblMapSize(sl) == 2);
    // 
    // Define timer to stop event loop after MAX_ACK_WAIT + 20%. This time need to run send list twice.
    ev_timer timeout_watcher;
    ev_timer_init (&timeout_watcher, timeout_cb, MAX_ACK_WAIT*1 + MAX_ACK_WAIT*0.2, 0.0);
    ev_timer_start (ke.ev_loop, &timeout_watcher);     
    // 
    // f) Start event loop
    ev_run (ke.ev_loop, 0);
    //
    // g) The sent packets will be resend by send list timer
    recvlen = recvfrom(fd_r, buf_recv, KSN_BUFFER_SIZE, 0, (__CONST_SOCKADDR_ARG) &addr_recv, &addr_recv_len);
    CU_ASSERT(tru_header->id == 0); // Packet id = 0
    recvlen = recvfrom(fd_r, buf_recv, KSN_BUFFER_SIZE, 0, (__CONST_SOCKADDR_ARG) &addr_recv, &addr_recv_len);
    CU_ASSERT(tru_header->id == 1); // Packet id = 1
    // Check send list - it should contain 2 records (with id = 0 and 1 and attempt 1)
    CU_ASSERT(pblMapSize(sl) == 2); // Send list size = 2
    sl_data *sl_d = ksnTRUDPsendListGetData(tu, 0, (__CONST_SOCKADDR_ARG)&addr_r); // Check id = 0
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl_d); // id =  0 present
    CU_ASSERT(sl_d->attempt == 1); // Number of attempt 1
    sl_d = ksnTRUDPsendListGetData(tu, 1, (__CONST_SOCKADDR_ARG)&addr_r); // Check id = 1
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl_d); // id =  1 present
    CU_ASSERT(sl_d->attempt == 1); // Number of attempt 1
    //
    // h) Send ACK from receiver to sender to remove one packet from send list
    tru_header->id = 1;
    tru_header->message_type = TRU_ACK;
    tru_header->payload_length = 0;
    tru_header->timestamp = 0;
    tru_header->version_major= 0;
    tru_header->version_minor = 0;
    sent = sendto(fd_r, tru_header, tru_ptr, 0, (__CONST_SOCKADDR_ARG)&addr_s, addr_len);
    CU_ASSERT_FATAL(sent > 0);
    recvlen = ksnTRUDPrecvfrom(tu, fd_s, buf_recv, KSN_BUFFER_SIZE, 0, (__CONST_SOCKADDR_ARG) &addr_recv, &addr_recv_len);
    CU_ASSERT(recvlen == 0);
    CU_ASSERT(pblMapSize(sl) == 1); // Send list size = 1
    sl_d = ksnTRUDPsendListGetData(tu, 0, (__CONST_SOCKADDR_ARG)&addr_r); // Check id = 0
    CU_ASSERT_PTR_NOT_NULL_FATAL(sl_d); // id =  0 present    
    //
    // j) Clear send list
    ksnTRUDPsendListRemoveAll(tu, sl);
    CU_ASSERT(pblMapSize(sl) == 0);
   
    // Stop test UDP sender & receiver
    close(fd_s);
    close(fd_r);
    
    // Destroy ksnTRUDPClass
    ksnTRUDPDestroy(tu);
    CU_PASS("Destroy ksnTRUDPClass done");    
}

// Test main RT-UDP function ksnTRUDPreceivefrom
void test_2_9() {
    
    // Emulate ksnCoreClass
    kc_emul();

    // Test constants and variables
    const size_t addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in addr_s, addr_r, addr_recv;
    int fd_s, fd_r, port_s = 9027, port_r = 9029;
    const char *addr_str = "127.0.0.1";
    size_t addr_recv_len = addr_len;
    char buf_send[KSN_BUFFER_SIZE];
    char buf_recv[KSN_BUFFER_SIZE];
    ksnTRUDP_header *tru_header = (ksnTRUDP_header *) buf_send;
    const size_t tru_ptr = sizeof (ksnTRUDP_header); // TR-UDP header size
    
    // Start test UDP sender
    fd_s = bind_udp(&port_s);
    CU_ASSERT(fd_s > 0);
    
    // Start test UDP receiver
    fd_r = bind_udp(&port_r);
    CU_ASSERT(fd_r > 0);
    
    // Fill sender address 
    if(inet_aton(addr_str, &addr_s.sin_addr) == 0) CU_ASSERT(1 == 0);
    addr_s.sin_family = AF_INET;
    addr_s.sin_port = htons(port_s);
    
    // Fill receiver address 
    if(inet_aton(addr_str, &addr_r.sin_addr) == 0) CU_ASSERT(1 == 0);
    addr_r.sin_family = AF_INET;
    addr_r.sin_port = htons(port_r);
    
    // Initialize ksnTRUDPClass
    ksnTRUDPClass *tu = ksnTRUDPinit(&kc); // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL(tu);
    
    // TODO: 1) test ksnTRUDPreceivefrom
    // a) Send DATA to receiver
    tru_header->id = 1;
    tru_header->message_type = TRU_DATA;
    const char *buf = "Hello world - 1"; // Data to send
    size_t buf_len = strlen(buf) + 1; // Size of send, data
    memcpy(tru_header + tru_ptr, buf, buf_len);
    tru_header->payload_length = buf_len;
    tru_header->timestamp = 0;
    tru_header->version_major= 0;
    tru_header->version_minor = 0;
    ssize_t sent = sendto(fd_s, tru_header, tru_ptr + buf_len, 0, (__CONST_SOCKADDR_ARG)&addr_r, addr_len);
    CU_ASSERT_FATAL(sent > 0);
    ssize_t recvlen = ksnTRUDPrecvfrom(tu, fd_r, buf_recv, KSN_BUFFER_SIZE, 0, (__CONST_SOCKADDR_ARG) &addr_recv, &addr_recv_len);
    CU_ASSERT(recvlen == 0);
    
    // Stop test UDP sender & receiver
    close(fd_s);
    close(fd_r);
    
    // Destroy ksnTRUDPClass
    ksnTRUDPDestroy(tu);
    CU_PASS("Destroy ksnTRUDPClass done");    
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
        (NULL == CU_add_test(pSuite, "RT-UDP reset functions", test_2_4)) ||
        (NULL == CU_add_test(pSuite, "RT-UDP send list functions", test_2_5)) ||
        (NULL == CU_add_test(pSuite, "RT-UDP send list timer functions", test_2_6)) ||
        (NULL == CU_add_test(pSuite, "RT-UDP receive heap functions", test_2_7)) ||
        (NULL == CU_add_test(pSuite, "RT-UDP sendto function", test_2_8)) ||
        (NULL == CU_add_test(pSuite, "RT-UDP recvfrom function", test_2_9))
            ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
