#include <stdio.h>
#include "text-filter.h"
int main(void) {
    char *test_log = "a b c d";
    char *match = "a&b";
    int res = log_string_match(test_log, match);
    printf("main.c: %d\n", res);
    return 0;
}
