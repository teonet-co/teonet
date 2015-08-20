/*
 * File:   test_pbl_kf.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on Aug 20, 2015, 7:14:55 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "ev_mgr.h"
#include "modules/pbl_kf.h"

/*
 * CUnit Test Suite
 */

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

#define kc_emul() \
  ksnetEvMgrClass ke_obj; \
  ksnetEvMgrClass *ke = &ke_obj

// Initialize/Destroy PBL KeyFile module
void test_3_1() {

    // Emulate ksnCoreClass
    kc_emul();

    // Initialize module
    ksnPblKfClass *kf = ksnPblKfInit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);
    CU_ASSERT_PTR_NULL(kf->namespace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Destroy module
    ksnPblKfDestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");
}

// Set default namespace
void test_3_2() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Remove test file if exist
    remove("test");
    
    // Initialize module
    ksnPblKfClass *kf = ksnPblKfInit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);
    CU_ASSERT_PTR_NULL(kf->namespace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Set default namespace - the file at disk should be created
    ksnPblKfNamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->namespace, "test");
    CU_ASSERT_PTR_NOT_NULL(kf->k);
    //if(kf->k == NULL) printf("pbl_errno: %d ...", pbl_errno);
    
    // Set test data
    int rv = ksnPblKfSet(kf, "test_key", "test data", 10);
    CU_ASSERT(rv == 0);
    
    // Set NULL namespace - the file should be flashed and closed
    ksnPblKfNamespaceSet(kf, NULL);
    CU_ASSERT_PTR_NULL(kf->namespace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Set default namespace again - the file at disk should be opened
    ksnPblKfNamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->namespace, "test");
    CU_ASSERT_PTR_NOT_NULL(kf->k);
    
    // Read test data
    size_t data_len;
    char *data = ksnPblKfGet(kf, "test_key", &data_len);
    CU_ASSERT_STRING_EQUAL(data, "test data");
    CU_ASSERT(data_len == 10);
    
    // Destroy module
    ksnPblKfDestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");   
    
    // Remove test file if exist
    remove("test");        
}

// Set and get data
void test_3_3() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Remove test file if exist
    remove("test");

    // Initialize module
    ksnPblKfClass *kf = ksnPblKfInit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);

    // Set default namespace - the file at disk should be created
    ksnPblKfNamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->namespace, "test");
    CU_ASSERT_PTR_NOT_NULL(kf->k);
    
    // Set test data
    int rv = ksnPblKfSet(kf, "test_key_01", "test data - 01", 15);
    CU_ASSERT(rv == 0);

    // Set test data
    rv = ksnPblKfSet(kf, "test_key_02", "test data - 02", 15);
    CU_ASSERT(rv == 0);

    // Set test data
    rv = ksnPblKfSet(kf, "test_key_03", "test data - 03", 15);
    CU_ASSERT(rv == 0);

    // Set test data
    rv = ksnPblKfSet(kf, "test_key_04", "test data - 04", 15);
    CU_ASSERT(rv == 0);

    // Read test data
    size_t data_len;
    char *data = ksnPblKfGet(kf, "test_key_03", &data_len);
    CU_ASSERT_STRING_EQUAL(data, "test data - 03");
    CU_ASSERT(data_len == 15);
    
    // Set test data with existing key - update the record
    rv = ksnPblKfSet(kf, "test_key_03", "test data - 03 - second", 24);
    CU_ASSERT(rv == 0);

    // Read test data
    data = ksnPblKfGet(kf, "test_key_03", &data_len);
    CU_ASSERT_STRING_EQUAL(data, "test data - 03 - second");
    CU_ASSERT(data_len == 24);

    // Destroy module
    ksnPblKfDestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");

    // Remove test file if exist
    //remove("test");
}

// Test template
void test_3_template() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Initialize module
    ksnPblKfClass *kf = ksnPblKfInit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);
    
    // Destroy module
    ksnPblKfDestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");
}


int main() {
    CU_pSuite pSuite = NULL;

    /* Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* Add a suite to the registry */
    pSuite = CU_add_suite("PBL KeyFile module functions", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "Initialize/Destroy module class", test_3_1)) ||
        (NULL == CU_add_test(pSuite, "Set default namespace", test_3_2)) ||
        (NULL == CU_add_test(pSuite, "Set and get data", test_3_3))) {
        
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
