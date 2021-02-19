/**
* \file text-filter.c
* \author max
* Created on Fri Feb 19 00:37:51 2021
*/
#include <stdio.h>
#include "text-filter.h"
int main(void) {
    char *test ="logvar=\"a b c d\"\na&b\n";
    int res = log_string_match(test);
    printf("%d\n", res);
    return 0;
}
