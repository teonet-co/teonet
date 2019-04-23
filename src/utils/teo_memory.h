/* 
 * \file:   teo_memory.h
 * \author: max <mpano91@gmail.com>
 *
 * Created on Apr 23, 2019, 7:52 PM
 */

#ifndef TEO_MEMORY_H
#define TEO_MEMORY_H
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

void *teo_malloc(size_t);
void *teo_calloc(size_t);
char *teo_strdup(const char *);
char *teo_strndup(char *, size_t);
void *teo_realloc(void *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* TEO_MEMORY_H */

