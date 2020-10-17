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
    CU_ASSERT_PTR_NULL(kf->defNameSpace);
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
    CU_ASSERT_PTR_NULL(kf->defNameSpace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Remove namespace if exist
    ksnTDBnamespaceRemove(kf, "test");
    
    // Set default namespace - the file at disk should be created
    ksnTDBnamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->defNameSpace, "test");
    CU_ASSERT_PTR_NOT_NULL(kf->k);
    //if(kf->k == NULL) printf("pbl_errno: %d ...", pbl_errno);
    
    // Set test data
    int rv = ksnTDBsetStr(kf, "test_key", "test data", 10);
    CU_ASSERT(rv == 0);
    
    // Set NULL namespace - the file should be flashed and closed
    ksnTDBnamespaceSet(kf, NULL);
    CU_ASSERT_PTR_NULL(kf->defNameSpace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Set default namespace again - the file at disk should be opened
    ksnTDBnamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->defNameSpace, "test");
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
    CU_ASSERT_STRING_EQUAL(kf->defNameSpace, "test");
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
    
    // Large data set
    char *ld = malloc(64*1024);
    rv = ksnTDBsetStr(kf, "test_key_04", ld, 64*1024);
    CU_ASSERT(rv == 0);
    
    // Large data delete
    rv = ksnTDBdeleteStr(kf, "test_key_04");
    CU_ASSERT(rv == 0);

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
    CU_ASSERT_PTR_NULL(kf->defNameSpace);
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

//! Get list of keys without default namespace
void test_3_5() {
    
    // Emulate ksnCoreClass
    kc_emul();
    
    // Initialize module
    ksnTDBClass *kf = ksnTDBinit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kf);
    CU_ASSERT_PTR_NULL(kf->defNameSpace);
    CU_ASSERT_PTR_NULL(kf->k);
    
    // Remove namespace if exist
    ksnTDBnamespaceRemove(kf, "test");
    
    // Set default namespace - the file at disk should be created
    ksnTDBnamespaceSet(kf, "test");
    CU_ASSERT_STRING_EQUAL(kf->defNameSpace, "test");
    CU_ASSERT_PTR_NOT_NULL(kf->k);
    
    // Set three keys
    //
    // Set test data
    int rv = ksnTDBsetStr(kf, "key_01", "test data - 01", 15);
    CU_ASSERT(rv == 0);
    //
    // Set test data
    rv = ksnTDBsetStr(kf, "key_02", "test data - 02", 15);
    CU_ASSERT(rv == 0);
    //
    // Set test data
    rv = ksnTDBsetStr(kf, "key_03", "test data - 03", 15);
    CU_ASSERT(rv == 0);
    
    // Get list of keys
    ksnet_stringArr argv = ksnet_stringArrCreate();
    int num_of_keys = ksnTDBkeyList(kf, NULL, &argv);
    CU_ASSERT(num_of_keys == 3);
    
    // Check string array
    int length_of_array = ksnet_stringArrLength(argv);
    CU_ASSERT(length_of_array == 3);
    CU_ASSERT(!strcmp(argv[0], "key_01"));
    CU_ASSERT(!strcmp(argv[1], "key_02"));
    CU_ASSERT(!strcmp(argv[2], "key_03"));
    
    // Combine arrays element to string
    char *ar_str = ksnet_stringArrCombine(argv, ", ");
    printf(" keys: %s ...", ar_str);
    free(ar_str);
    
    // Free string array
    ksnet_stringArrFree(&argv);
    CU_ASSERT(argv == NULL);
    
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
        (NULL == CU_add_test(pSuite, "Set and get data without default namespace", test_3_4)) ||
        (NULL == CU_add_test(pSuite, "Get list of keys without default namespace", test_3_5))) {
        
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    return 0;
}
