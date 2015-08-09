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

// Modules functions
int add_suite_1_tests(void);
int add_suite_2_tests(void);

// Global variables
CU_pSuite pSuite = NULL;


/*
 * CUnit Test 
 */

int init_suite(void) {    
    return 0;
}

int clean_suite(void) {
    return 0;
}

int main() {
    
    KSN_SET_TEST_MODE(1);
            
    // Initialize the CUnit test registry 
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    // Add a suite to the registry 
    pSuite = CU_add_suite("Teonet library Crypt module", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    add_suite_1_tests();    
    
    // Add a suite to the registry 
    pSuite = CU_add_suite("Teonet library TR-UDP module", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    add_suite_2_tests();

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    //CU_list_tests_to_file();
    CU_basic_run_tests();
    //CU_console_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
