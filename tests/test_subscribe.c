/*
 * File:   test_subscribe.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on Jan 2, 2016, 5:20:16 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "ev_mgr.h"
#include "modules/cque.h"

extern CU_pSuite pSuite; // Test global variable

int teoSScrNumberOfSubscribers(teoSScrClass *sscr);

/**
 * Emulate ksnetEvMgrClass
 */
#define kc_emul() \
  ksnetEvMgrClass ke_obj; \
  ksnetEvMgrClass *ke = &ke_obj; \
  ke->ev_loop = ev_loop_new (0); \
  memset(&ke_obj.ksn_cfg, 0 , sizeof(ke_obj.ksn_cfg))

/*
 * CUnit Test Suite
 */

/**
 * Initialize/Destroy module class
 */
void test_6_1() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Initialize module
    teoSScrClass* ksscr = teoSScrInit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ksscr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ksscr->map);
    
    // Destroy module
    teoSScrDestroy(ksscr);
    CU_PASS("Destroy teoSScrClass done");
}

/**
 * Test Subscribe/UnSubscribe
 */
void test_6_2() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    teoSScrClass* sscr = teoSScrInit(ke);
    char *peer_name = "test-peer";
    char *peer_name_2 = "test-peer_2";
    
    // Subscribe
    teoSScrSubscription(sscr, peer_name, 100);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 1);
    
    teoSScrSubscription(sscr, peer_name, 110);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 2);
    
    teoSScrSubscription(sscr, peer_name_2, 110);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 3);
    
    teoSScrSubscription(sscr, peer_name_2, 100);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 4);
    
    // UnSubscribe
    teoSScrUnSubscription(sscr, peer_name, 100);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 3);
    
    teoSScrUnSubscription(sscr, peer_name_2, 100);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 2);
    
    teoSScrUnSubscription(sscr, peer_name, 110);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 1);
    
    teoSScrUnSubscription(sscr, peer_name_2, 110);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 0);       
    
    
    // Subscribe
    teoSScrSubscription(sscr, peer_name, 100);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 1);
    
    teoSScrSubscription(sscr, peer_name, 110);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 2);
    
    teoSScrSubscription(sscr, peer_name, 120);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 3);
    
    teoSScrSubscription(sscr, peer_name, 130);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 4);
    
    // UnSubscribe All
    teoSScrUnSubscriptionAll(sscr, peer_name);
    CU_ASSERT(teoSScrNumberOfSubscribers(sscr) == 0);
    
    teoSScrDestroy(sscr);
}

/**
 * Add Subscribe module tests
 * 
 * @return 
 */
int add_suite_6_tests(void) {
    
    // Add the tests to the suite 
    if ((NULL == CU_add_test(pSuite, "Initialize/Destroy module class", test_6_1)) ||
        (NULL == CU_add_test(pSuite, "Subscribe/UnSubscribe", test_6_2))) {
        
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    return 0;
}
