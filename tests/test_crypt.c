/*
 * File:   test_crypt.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Crypt module test
 *
 * Created on Aug 7, 2015, 9:31:12 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "ev_mgr.h"
#include "crypt.h"

extern CU_pSuite pSuite; // Test global variable

extern int num_crypt_module; // Teonet crypt module global variable

/**
 * Test Initialize/Destroy Crypt module
 */
void test_1_1() {
    
    ksnCryptClass *kcr;
    CU_ASSERT_PTR_NOT_NULL_FATAL((kcr = ksnCryptInit(NULL)));
    CU_ASSERT(num_crypt_module == 1);
    ksnCryptDestroy(kcr);
    CU_ASSERT(num_crypt_module == 0);
}

/**
 * Add Crypt suite tests
 * 
 * @return 
 */
int add_suite_1_tests(void) {
    
    // Add the tests to the suite 
    if ((NULL == CU_add_test(pSuite, "Initialize/Destroy Crypt module", test_1_1))
            ) {       
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    return 0;
}
