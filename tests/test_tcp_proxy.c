/*
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

size_t ksnTCPProxyPackageCreate(ksnTCPProxyClass *tp, void *buffer, 
        size_t buffer_len, const char *addr, int port, const void *data, 
        size_t data_len);

uint8_t ksnTCPProxyChecksumCalculate(void *data, size_t data_len);

/**
 * Emulate ksnetEvMgrClass
 */
#define ke_emul() \
  ksnetEvMgrClass ke_obj; \
  ksnetEvMgrClass *ke = &ke_obj; \
  ke->ksn_cfg.tcp_allow_f = 0

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
    
    // 1) Create package check
    const int port = 9090;
    const char *addr = "127.0.0.1";
    const char *data = "Some package data";
    const size_t data_len = strlen(data) + 1;
    size_t buffer_len = KSN_BUFFER_DB_SIZE;
    char buffer[buffer_len];
    // Create package
    size_t pl = ksnTCPProxyPackageCreate(tp, buffer, buffer_len, addr, port, data, data_len);
    // Check result
    ksnTCPProxyMessage_header *th = (ksnTCPProxyMessage_header*) buffer;
    CU_ASSERT(pl > 0); // Packet created (the buffer has enough size)
    CU_ASSERT_EQUAL(th->checksum, ksnTCPProxyChecksumCalculate((void*)buffer, pl)); // Check checksum
    CU_ASSERT_EQUAL(th->addr_len, strlen(addr) + 1); // Check address string length
    CU_ASSERT_EQUAL(th->port, port); // Check port number
    CU_ASSERT_STRING_EQUAL(buffer + sizeof(ksnTCPProxyMessage_header), addr); // Check address
    CU_ASSERT_EQUAL(th->package_len, data_len); // Check package length
    CU_ASSERT_STRING_EQUAL(buffer + sizeof(ksnTCPProxyMessage_header) + th->addr_len, data); // Check package
    CU_ASSERT_EQUAL(pl, sizeof(ksnTCPProxyMessage_header) + th->addr_len + data_len); // Check package length
    
    // 2) Check buffer size errors
    // Create package with buffer less than header size
    size_t err_pl = ksnTCPProxyPackageCreate(tp, buffer, sizeof(ksnTCPProxyMessage_header) - 1, addr, port, data, data_len);
    CU_ASSERT_EQUAL(err_pl, -1);
    // Create package with buffer less than package size
    err_pl = ksnTCPProxyPackageCreate(tp, buffer, pl - 1, addr, port, data, data_len);
    CU_ASSERT_EQUAL(err_pl, -2);

            
    // N) \todo Process package
    
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
