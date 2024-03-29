/**
 * 
 * \file   test_teonet.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet cUnit test (main test module)
 * 
 * cUnit test main module code:  \include test_teonet.c
 *
 * Created on Aug 7, 2015, 9:31:12 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "ev_mgr.h"

// Modules functions
int add_suite_1_tests(void);
int add_suite_3_tests(void);
int add_suite_4_tests(void);
int add_suite_5_tests(void);
int add_suite_6_tests(void);
int add_suite_filter_tests(void);

// Global variables
CU_pSuite pSuite = NULL;


/*
 * CUnit Test 
 */

/**
 * Initialize suite
 */
int init_suite(void) {
    return 0;
}

/**
 */
int clean_suite(void) {
    return 0;
}

//! cUnit test main function
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
    
    // Add a suite to the registry
    pSuite = CU_add_suite("Teonet DB based at PBL KeyFile module functions", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    add_suite_3_tests();

    // Add a suite to the registry
    pSuite = CU_add_suite("Callback QUEUE module functions", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    add_suite_4_tests();

    // Add a suite to the registry
    pSuite = CU_add_suite("TCP Proxy module functions", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    add_suite_5_tests();

    // Add a suite to the registry
    pSuite = CU_add_suite("Subscribe module functions", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    add_suite_6_tests();

    // Add a suite to the registry
    pSuite = CU_add_suite("Text-filter module functions", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    add_suite_filter_tests();

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    //CU_list_tests_to_file();
    CU_basic_run_tests();
    //CU_console_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
