/*
 * File:   test_cque.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on Aug 22, 2015, 3:35:32 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

/*
 * CUnit Test Suite
 */

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

//Initialize/Destroy module class
void test_4_1() {
    CU_ASSERT(2 * 2 == 4);
}

void test_4_2() {
    CU_ASSERT(2 * 2 == 5);
}

int main() {
    CU_pSuite pSuite = NULL;

    /* Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* Add a suite to the registry */
    pSuite = CU_add_suite("Callback QUEUE module functions", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "Initialize/Destroy module class", test_4_1)) ||
            (NULL == CU_add_test(pSuite, "test2", test_4_2))) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
