/*
 * \file   test_teodb.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet DB based at PBL KeyFile [module](@ref teodb.c) tests suite
 * 
 * Tests Teonet DB functions:
 * 
 * * Initialize/Destroy Teonet DB module: test_3_1()
 * * Set default namespace: test_3_2()
 * * Set and get data: test_3_3()
 * * Set and get data without default namespace: test_3_4()
 * 
 * cUnit test suite code: \include test_teodb.c
 * 
 * Created on Aug 20, 2015, 7:14:55 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "ev_mgr.h"
#include "modules/teodb.h"

extern CU_pSuite pSuite; // Test global variable

#define kc_emul() \
  ksnetEvMgrClass ke_obj; \
  ksnetEvMgrClass *ke = &ke_obj

//! Initialize/Destroy Teonet DB module
void test_3_1() {

    // Emulate ksnCoreClass
    kc_emul();

    // Initialize module
    ksnTDBClass *kf = ksnTDBinit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);
    CU_ASSERT_PTR_NULL(kf->namespace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Destroy module
    ksnTDBdestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");
}

//! Set default namespace
void test_3_2() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Initialize module
    ksnTDBClass *kf = ksnTDBinit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);
    CU_ASSERT_PTR_NULL(kf->namespace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Remove namespace if exist
    ksnTDBnamespaceRemove(kf, "test");
    
    // Set default namespace - the file at disk should be created
    ksnTDBnamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->namespace, "test");
    CU_ASSERT_PTR_NOT_NULL(kf->k);
    //if(kf->k == NULL) printf("pbl_errno: %d ...", pbl_errno);
    
    // Set test data
    int rv = ksnTDBsetStr(kf, "test_key", "test data", 10);
    CU_ASSERT(rv == 0);
    
    // Set NULL namespace - the file should be flashed and closed
    ksnTDBnamespaceSet(kf, NULL);
    CU_ASSERT_PTR_NULL(kf->namespace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Set default namespace again - the file at disk should be opened
    ksnTDBnamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->namespace, "test");
    CU_ASSERT_PTR_NOT_NULL(kf->k);
    
    // Read test data
    size_t data_len;
    char *data = ksnTDBgetStr(kf, "test_key", &data_len);
    CU_ASSERT_STRING_EQUAL(data, "test data");
    CU_ASSERT(data_len == 10);
    
    // Remove namespace if exist
    ksnTDBnamespaceRemove(kf, "test");
    
    // Destroy module
    ksnTDBdestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");       
}

//! Set and get data
void test_3_3() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Initialize module
    ksnTDBClass *kf = ksnTDBinit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);

    // Remove namespace if exist
    ksnTDBnamespaceRemove(kf, "test");

    // Set default namespace - the file at disk should be created
    ksnTDBnamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->namespace, "test");
    CU_ASSERT_PTR_NOT_NULL(kf->k);
    
    // Set test data
    int rv = ksnTDBsetStr(kf, "test_key_01", "test data - 01", 15);
    CU_ASSERT(rv == 0);

    // Set test data
    rv = ksnTDBsetStr(kf, "test_key_02", "test data - 02", 15);
    CU_ASSERT(rv == 0);

    // Set test data
    rv = ksnTDBsetStr(kf, "test_key_03", "test data - 03", 15);
    CU_ASSERT(rv == 0);

    // Set test data
    rv = ksnTDBsetStr(kf, "test_key_04", "test data - 04", 15);
    CU_ASSERT(rv == 0);

    // Read test data
    size_t data_len;
    char *data = ksnTDBgetStr(kf, "test_key_03", &data_len);
    CU_ASSERT_STRING_EQUAL(data, "test data - 03");
    CU_ASSERT(data_len == 15);
    
    // Update test data with existing key
    rv = ksnTDBsetStr(kf, "test_key_03", "test data - 03 - second", 24);
    CU_ASSERT(rv == 0);

    // Read test data
    data = ksnTDBgetStr(kf, "test_key_03", &data_len);
    CU_ASSERT_STRING_EQUAL(data, "test data - 03 - second");
    CU_ASSERT(data_len == 24);
    
    // Delete all records with key
    rv = ksnTDBdeleteStr(kf, "test_key_03");
    CU_ASSERT(rv == 0);
    
    // Try to read deleted data
    data = ksnTDBgetStr(kf, "test_key_03", &data_len);
    CU_ASSERT_PTR_NULL(data);
    CU_ASSERT(data_len == 0);

    // Remove namespace if exist
    ksnTDBnamespaceRemove(kf, "test");  
    
    // Destroy module
    ksnTDBdestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");
}

//! Set and get data without default namespace
void test_3_4() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Initialize module
    ksnTDBClass *kf = ksnTDBinit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);
    CU_ASSERT_PTR_NULL(kf->namespace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Remove namespace if exist
    ksnTDBnamespaceRemove(kf, "test");
    
    // Set test data
    int rv = ksnTDBsetNsStr(kf, "test", "test_key", "test data", 10);
    CU_ASSERT(rv == 0);    
    
    // Read test data
    size_t data_len;
    char *data = ksnTDBgetNsStr(kf, "test", "test_key", &data_len);
    CU_ASSERT_STRING_EQUAL(data, "test data");
    CU_ASSERT(data_len == 10);
    
    // Update test data
    rv = ksnTDBsetNsStr(kf, "test", "test_key", "test data updated", 18);
    CU_ASSERT(rv == 0);    
    
    // Read updated test data
    data = ksnTDBgetNsStr(kf, "test", "test_key", &data_len);
    CU_ASSERT_STRING_EQUAL(data, "test data updated");
    CU_ASSERT(data_len == 18);
    
    // Delete all records with key
    rv = ksnTDBdeleteNsStr(kf, "test", "test_key");
    CU_ASSERT(rv == 0);
    
    // Try to read deleted data
    data = ksnTDBgetNsStr(kf, "test", "test_key", &data_len);
    CU_ASSERT_PTR_NULL(data);
    CU_ASSERT(data_len == 0);
    
    // Remove namespace if exist
    ksnTDBnamespaceRemove(kf, "test");
    
    // Destroy module
    ksnTDBdestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");
}

//! Test template
void test_3_template() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Initialize module
    ksnTDBClass *kf = ksnTDBinit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);
    
    // Destroy module
    ksnTDBdestroy(kf);
    CU_PASS("Destroy ksnPblKfClass done");
}


/**
 * Add Teonet DB module tests
 * 
 * @return 
 */
int add_suite_3_tests(void) {
    
    // Add the tests to the suite 
    if ((NULL == CU_add_test(pSuite, "Initialize/Destroy module class", test_3_1)) ||
        (NULL == CU_add_test(pSuite, "Set default namespace", test_3_2)) ||
        (NULL == CU_add_test(pSuite, "Set and get data with default namespace", test_3_3)) ||
        (NULL == CU_add_test(pSuite, "Set and get data without default namespace", test_3_4))) {
        
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    return 0;
}
