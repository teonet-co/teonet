/*
 * File:   test_subscribe.c
 * Author: max <mpano91@gmail.com>
 *
 * Created on Arl 4, 2021, 5:20:16 AM
 */

#include <stdio.h>
#include <CUnit/Basic.h>
#include "text-filter/text-filter.h"

extern CU_pSuite pSuite;

void test_filter_1() {
    char *test_log = "a b c d";
    char *match_1 = "a&b";
    int res = log_string_match(test_log, match_1);
    CU_ASSERT(res == 1);
    char *match_2 = "a&h";
    res = log_string_match(test_log, match_2);
    CU_ASSERT(res == 0);
}

void test_filter_2() {
    char *test_log_1 = "[2021-04-07 11:49:18:442] DEBUG_VV net_core: ksnCoreSendto:(net_core.c:250): send 17 bytes data, cmd 65 to ::ffff:127.0.0.1:9010";
    char *test_log_2 = "[2021-04-07 11:49:18:442] DEBUG_VV net_crypt: ksnEncryptPackage:(crypt.c:258): encrypt 26 bytes to 32 bytes buffer";
    char *match_1 = "(net_core|net_crypt)&(!tr_udp)";
    int res = log_string_match(test_log_1, match_1);
    res *= log_string_match(test_log_2, match_1);
    CU_ASSERT(res == 1);
}


int add_suite_filter_tests(void) {
    if ((NULL == CU_add_test(pSuite, "Text filter test-1", test_filter_1)) ||
        (NULL == CU_add_test(pSuite, "Text filter test-2", test_filter_2))
        
        ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    return 0;
}