/**
 * \file   test_tcp_proxy.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet TCP Proxy [module](@ref tcp_proxy.c) tests suite
 * 
 * Tests TCP Proxy functions:
 * 
 * * Initialize/Destroy TCP Proxy module: test_5_1()
 * * Create and Process TCP Proxy Package functions: test_5_2()
 * 
 * cUnit test suite code: \include test_tcp_proxy.c
 * 
 * Created on Sep 8, 2015, 11:10:22 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "ev_mgr.h"
#include "modules/tcp_proxy.h"

extern CU_pSuite pSuite;

size_t ksnTCPProxyPackageCreate(void *buffer, size_t buffer_len, 
        const char *addr, int port, int cmd, const void *data, size_t data_len);
uint8_t teoByteChecksum(void *data, size_t data_len);
int ksnTCPProxyPackageProcess(ksnTCPProxyPacketData *packet, void *data, 
        size_t data_length);

/**
 * Emulate ksnetEvMgrClass
 */
#define ke_emul() \
  ksnetEvMgrClass ke_obj; \
  ksnetEvMgrClass *ke = &ke_obj; \
  ke->ksn_cfg.tcp_allow_f = 0; \
  ke->ksn_cfg.r_tcp_f = 0

//! Initialize/Destroy TCP Proxy module
void test_5_1() {
    
    // Emulate ksnCoreClass
    ke_emul();

    ksnTCPProxyClass *tp; // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL((tp = ksnTCPProxyInit(ke)));
    ksnTCPProxyDestroy(tp); // Destroy ksnTRUDPClass
    CU_PASS("Destroy ksnTCPProxyClass done");
}

//! Create and Process TCP Proxy Package functions
void test_5_2() {
    
    // Emulate ksnCoreClass
    ke_emul();

    // Initialize ksnTRUDPClass
    ksnTCPProxyClass *tp; 
    CU_ASSERT_PTR_NOT_NULL_FATAL((tp = ksnTCPProxyInit(ke)));
    
    ksnTCPProxyData _tpd;
    ksnTCPProxyData *tpd = &_tpd;
    // Initialize input packet buffer parameters
    tpd->packet.ptr = 0; // Pointer to data end in packet buffer
    tpd->packet.length = 0; // Length of received packet
    tpd->packet.stage = WAIT_FOR_START; // Stage of receiving packet
    tpd->packet.header = (ksnTCPProxyHeader*) tpd->packet.buffer; // Pointer to packet header
    
    // 1) Create package function check
    const int port = 9090;
    const char *addr = "127.0.0.1";
    const char *data = "Some package data (1) ...";
    const char *data2 = "Some package data (2) with additional text ...";
    const size_t data_len = strlen(data) + 1;
    const size_t data2_len = strlen(data2) + 1;
    size_t buffer_len = KSN_BUFFER_DB_SIZE;
    char buffer[buffer_len];
    // Create package
    size_t pl = ksnTCPProxyPackageCreate(buffer, buffer_len, addr, port, CMD_TCPP_PROXY, data, data_len);
    // Check result
    ksnTCPProxyHeader *th = (ksnTCPProxyHeader*) buffer;
    CU_ASSERT(pl > 0); // Packet created (the buffer has enough size)
    CU_ASSERT_EQUAL(th->checksum, teoByteChecksum((void*)buffer + 1, pl - 1)); // Check checksum
    CU_ASSERT_EQUAL(th->addr_length, strlen(addr) + 1); // Check address string length
    CU_ASSERT_EQUAL(th->port, port); // Check port number
    CU_ASSERT_STRING_EQUAL(buffer + sizeof(ksnTCPProxyHeader), addr); // Check address
    CU_ASSERT_EQUAL(th->packet_length, data_len); // Check package length
    CU_ASSERT_STRING_EQUAL(buffer + sizeof(ksnTCPProxyHeader) + th->addr_length, data); // Check package
    CU_ASSERT_EQUAL(pl, sizeof(ksnTCPProxyHeader) + th->addr_length + data_len); // Check package length
    
    // 2) Check Create package function buffer size errors
    // Create package with buffer less than header size
    size_t pl_err = ksnTCPProxyPackageCreate(buffer, sizeof(ksnTCPProxyHeader) - 1, addr, port, CMD_TCPP_PROXY, data, data_len);
    CU_ASSERT_EQUAL(pl_err, -1);
    // Create package with buffer less than package size
    pl_err = ksnTCPProxyPackageCreate(buffer, pl - 1, addr, port, CMD_TCPP_PROXY, data, data_len);
    CU_ASSERT_EQUAL(pl_err, -2);
           
    // 3) Process package function
    // Create regular package
    pl = ksnTCPProxyPackageCreate(buffer, buffer_len, addr, port, CMD_TCPP_PROXY, data, data_len);
    CU_ASSERT(pl > 0); // Packet created (the buffer has enough size)
    // Process package
    int rv = ksnTCPProxyPackageProcess(&tpd->packet, buffer, pl);
    // Check saved buffer header parameters
    CU_ASSERT(rv > 0); // Packet process successfully
    //CU_ASSERT(pl > 0); // Packet created (the buffer has enough size)
    CU_ASSERT_EQUAL(tpd->packet.header->checksum, teoByteChecksum((void*)tpd->packet.buffer + 1, pl - 1)); // Check checksum
    CU_ASSERT_EQUAL(tpd->packet.header->addr_length, strlen(addr) + 1); // Check address string length
    CU_ASSERT_EQUAL(tpd->packet.header->port, port); // Check port number
    CU_ASSERT_STRING_EQUAL(tpd->packet.buffer + sizeof(ksnTCPProxyHeader), addr); // Check address
    CU_ASSERT_EQUAL(tpd->packet.header->packet_length, data_len); // Check package length
    CU_ASSERT_STRING_EQUAL(tpd->packet.buffer + sizeof(ksnTCPProxyHeader) + tpd->packet.header->addr_length, data); // Check package
    CU_ASSERT_EQUAL(rv, pl); // Check package length
    
    // 4) Check receiving two packets which was combined into one buffer by small parts
    // Create buffer with two packet
    size_t pl_comb = 0; // Combined packet size
    int num_pack = 0; // Number of packets
    pl = ksnTCPProxyPackageCreate(buffer, buffer_len, addr, port, CMD_TCPP_PROXY, data, data_len);
    CU_ASSERT(pl > 0); // Packet created (the buffer has enough size)
//    printf(" pl = %d,", (int) pl );
    pl_comb += pl;
    num_pack++;
    pl = ksnTCPProxyPackageCreate(buffer + pl, buffer_len - pl, addr, port, CMD_TCPP_PROXY, data2, data2_len);
    CU_ASSERT(pl > 0); // Packet created (the buffer has enough size)
//    printf(" pl = %d,", (int) pl );    
    pl_comb += pl;
    num_pack++;
    // Process combined buffer
    int i;
    size_t ptr = 0; // Pointer in combined buffer
    
    size_t num_processed = 0; // Number of processed packets
    size_t len = pl_comb / 10;      
//    printf(" pl_comb = %d, len = %d, i:", (int) pl_comb, (int) len );
    for(i = 0; ptr < pl_comb; i++) {
        
//        printf(" %d", i+1);

        // Send part of buffer to Package Process 
        if( (len + ptr) > pl_comb ) {
            len = pl_comb - ptr;
//            printf(", final len = %d", len);
        }
  
        if(len > 0) {
            rv = ksnTCPProxyPackageProcess(&tpd->packet, buffer + ptr, len);
            CU_ASSERT(rv >= 0);
            if(rv > 0) {                
//                printf(", processed = %d, ptr = %d", rv, tp->packet.ptr);            
                CU_ASSERT_EQUAL(tpd->packet.header->checksum, teoByteChecksum((void*)tpd->packet.buffer + 1, rv - 1)); // Check checksum
                CU_ASSERT_EQUAL(tpd->packet.header->addr_length, strlen(addr) + 1); // Check address string length
                CU_ASSERT_EQUAL(tpd->packet.header->port, port); // Check port number
                CU_ASSERT_STRING_EQUAL(tpd->packet.buffer + sizeof(ksnTCPProxyHeader), addr); // Check address
                num_processed++;
            }
            ptr += len;
        }
    }
//    printf(", num_processed = %d ", num_processed);
    CU_ASSERT(num_processed == num_pack);
    
    // 5) Check receiving two packets which was combined into one buffer by one buffer
    // Create buffer with two packet
    pl_comb = 0; // Combined packet size
    num_pack = 0; // Number of packets
    num_processed = 0; // Number of processed packets
    pl = ksnTCPProxyPackageCreate(buffer, buffer_len, addr, port, CMD_TCPP_PROXY, data, data_len);
    CU_ASSERT(pl > 0); // Packet created (the buffer has enough size)
//    printf(" pl = %d,", (int) pl );
    pl_comb += pl;
    num_pack++;
    pl = ksnTCPProxyPackageCreate(buffer + pl, buffer_len - pl, addr, port, CMD_TCPP_PROXY, data2, data2_len);
    CU_ASSERT(pl > 0); // Packet created (the buffer has enough size)
//    printf(" pl = %d,", (int) pl );    
    pl_comb += pl;
    num_pack++;
    
    // Process first packet in combined buffer
    rv = ksnTCPProxyPackageProcess(&tpd->packet, buffer, pl_comb);
    CU_ASSERT(rv >= 0);
    if(rv > 0) {
        
        num_processed++;
        
//        printf(" processed = %d, ptr = %d", rv, tp->packet.ptr); 
    
        // Process next packets in combined buffer
        while((pl_comb - rv) > 0) {

            pl_comb -= rv;
            rv = ksnTCPProxyPackageProcess(&tpd->packet, buffer, 0);
            CU_ASSERT(rv >= 0);
            if(rv > 0) {
//                printf(" processed = %d, ptr = %d", rv, tp->packet.ptr); 
                num_processed++;
            }
        }
        
//        printf(", num_processed = %d ", num_processed);
    }
    
    // Destroy ksnTRUDPClass
    ksnTCPProxyDestroy(tp); 
    CU_PASS("Destroy ksnTCPProxyClass done");
}

/**
 * Add TCP Proxy suite tests
 *
 * @return
 */
int add_suite_5_tests(void) {

    // Add the tests to the suite
    if ((NULL == CU_add_test(pSuite, "Initialize/Destroy TCP Proxy module", test_5_1))
     || (NULL == CU_add_test(pSuite, "Create and Process TCP Proxy Package functions", test_5_2))
            ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
