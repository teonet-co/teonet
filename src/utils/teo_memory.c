/* 
 * \file:   teo_memory.c
 * \author: max <mpano91@gmail.com>
 *
 * Created on June 27, 2018, 4:18 PM
 */
#define _POSIX_C_SOURCE 200809L // strdup warning

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static
void teo_memory_out(void) {
    (void)fprintf(stderr, "Teonet. Out of memory\n");
    exit(1);
}


void *teo_malloc(size_t size) {
    void *memory;
    if((memory = malloc(size)) == NULL) {
        teo_memory_out();
    }
    
    return memory;
}


void *teo_calloc(size_t size) {
    void *memory;
    memory = teo_malloc(size);
    memset(memory, 0, size);

    return memory;
}


char *teo_strdup(const char *str) {
    char *newstr;
    if ((newstr = strdup(str)) == NULL) {
        teo_memory_out();
    }

    return newstr;
}


char *teo_strndup(char *str, size_t len) {
    char *newstr;
    if ((newstr = malloc(len + 1)) == NULL) {
        teo_memory_out();
    }

    (void)strncpy(newstr, str, len);
    newstr[len] = '\0';

    return (newstr);
}


void *teo_realloc(void *ptr, size_t size) {
    void *memory;
    if ((memory = realloc(ptr, size)) == NULL) {
        teo_memory_out();
    }

    return memory;
}
