/*
 * File:   string_arr.h
 * Author: Kirill Scherba
 *
 * Created on June 19, 2014, 7:43 AM
 */

#ifndef STRING_ARR_H
#define	STRING_ARR_H

typedef char ** ksnet_stringArr;

#ifdef	__cplusplus
extern "C" {
#endif

ksnet_stringArr ksnet_stringArrCreate();
ksnet_stringArr ksnet_stringArrAdd(ksnet_stringArr *arr, const char* str);
int             ksnet_stringArrLength(ksnet_stringArr arr);
ksnet_stringArr ksnet_stringArrFree(ksnet_stringArr *arr);
ksnet_stringArr	ksnet_stringArrSplit(const char* string, const char* separators,
                                int with_empty, int max_parts);
char *ksnet_stringArrCombine(ksnet_stringArr arr, const char* separator);
int ksnet_stringArrMoveTo(ksnet_stringArr arr, unsigned int fromIdx, unsigned int toIdx);

#ifdef	__cplusplus
}
#endif

#endif	/* STRING_ARR_H */

