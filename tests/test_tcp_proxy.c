/*
 * \file   test_tcp_proxy.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet TCP Proxy [module](@ref tcp_proxy.c) tests suite
 * 
 * Tests TCP Proxy functions:
 * 
 * * Initialize/Destroy TCP Proxy module: test_5_1()
 * * Create and Process Package functions: test_5_2()
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

/**
 * Emulate ksnetEvMgrClass
 */
#define ke_emul() \
  ksnetEvMgrClass ke_obj; \
  ksnetEvMgrClass *ke = &ke_obj

//! Initialize/Destroy TCP Proxy module
void test_5_1() {
    
    // Emulate ksnCoreClass
    ke_emul();

    ksnTCPProxyClass *tp; // Initialize ksnTRUDPClass
    CU_ASSERT_PTR_NOT_NULL_FATAL((tp = ksnTCPProxyInit(ke)));
    ksnTCPProxyDestroy(tp); // Destroy ksnTRUDPClass
    CU_PASS("Destroy ksnTCPProxyClass done");
}

//! Create and Process Package functions
void test_5_2() {
    
    // Emulate ksnCoreClass
    ke_emul();

    // Initialize ksnTRUDPClass
    ksnTCPProxyClass *tp; 
    CU_ASSERT_PTR_NOT_NULL_FATAL((tp = ksnTCPProxyInit(ke)));
    
    // 1) \todo Create package 
    
    // 2) \todo Process package
    
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
     || (NULL == CU_add_test(pSuite, "Create and Process Package functions", test_5_2))
            ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}
