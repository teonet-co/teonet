/**
* \file text-filter.c
* \author max
* Created on Fri Feb 19 00:37:51 2021
*/
#include <stdio.h>
#include "text-filter.h"

int main(void) {
    char *test_log = "a b c d";
    char *match = "a&b";
    int res = log_string_match(test_log, match);
    printf("text-filter.c: %d\n", res);
    return 0;
}