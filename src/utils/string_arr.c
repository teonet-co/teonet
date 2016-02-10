/**
 * C string array manage function
 */

#include <stdlib.h>
#include <string.h>

#include "string_arr.h"

// Local functions
static char *remove_extras(char *str, const char *extras);
static char find_replace_to_char(const char *str, const char *replace_to_str);
static int replace_char_between_quotas(char *str, char replace_from, char replace_to);

/**
 * Create empty c string array
 *
 * @return
 */
ksnet_stringArr ksnet_stringArrCreate() {

    return NULL;
}

/**
 * Add c string to string array
 *
 * @param str
 * @param arr
 * @return
 */
ksnet_stringArr ksnet_stringArrAdd(ksnet_stringArr *arr, const char* str) {

    int i = ksnet_stringArrLength(*arr);
    if(!i) *arr = malloc(sizeof(ksnet_stringArr) * 2);
    else *arr = realloc(*arr, sizeof(ksnet_stringArr) * (i+2) );

    (*arr)[i] = malloc(strlen(str) + 1);
    strcpy((*arr)[i], str);
    (*arr)[i+1] = NULL;

    return *arr;
}

/**
 * Get length of string array
 *
 * @param arr
 * @return
 */
int ksnet_stringArrLength(ksnet_stringArr arr) {

    int len = 0;

    if(arr != NULL) {

            while (arr[len] != NULL) len++;
    }

    return len;
}

/**
 * Move array item from one position to another
 * 
 * @param arr
 * @param fromIdx
 * @param toIdx
 * 
 * @return True at success, null at error
 */
int ksnet_stringArrMoveTo(ksnet_stringArr arr, unsigned int fromIdx, unsigned toIdx) {
    
    int i, retval = 0, len = ksnet_stringArrLength(arr);
    if(len && fromIdx < len && toIdx < len && fromIdx != toIdx) {
        
        if(fromIdx < toIdx) {
            char *from = arr[fromIdx];
            for(i = fromIdx + 1; i < len; i++) {
                arr[i-1] = arr[i];
                if(i == toIdx) {
                    arr[i] = from;
                    retval = 1;
                    break;
                }
            }
        }
        
        else {
            char *from = arr[fromIdx];
            for(i = fromIdx - 1; i >= 0; i--) {
                arr[i+1] = arr[i];
                if(i == toIdx) {
                    arr[i] = from;
                    retval = 1;
                    break;
                }
            }
        }
    }
    
    return retval;
}

/**
 * Free string array
 * @param arr
 */
ksnet_stringArr ksnet_stringArrFree(ksnet_stringArr *arr) {

    if(*arr != NULL) {

        int i;

        for(i = 0; (*arr)[i] != NULL; i++) free((*arr)[i]);
        free(*arr);

        *arr = NULL;
    }

    return *arr;
}


/**
 * Split string by separators into words
 *
 * @param string
 * @param separators
 * @param with_empty
 * @param max_parts Number of parts. if max_parts == 0 then there isn't parts
 *        limit
 *
 * @return String array. Should be free with free_string_array after using
 */
ksnet_stringArr ksnet_stringArrSplit(const char* string, const char* separators,
                             int with_empty, int max_parts) {

    #define MAX_STRING_LEN 1024

    #define arr_realloc(_len_) \
        result_len = _len_; \
        result = realloc(result, sizeof(char*) * result_len)

    #define arr_inc_size(_pos_) \
        if(_pos_ >= result_len) { \
            arr_realloc(result_len + MAX_STRING_LEN); \
        }

    if(string != NULL && separators != NULL) {

        int result_len = MAX_STRING_LEN;
        ksnet_stringArr result = malloc(sizeof(char*) * result_len);
        int pos = 0;

        if(string[0] == '\0') {
            result[pos] = NULL;
            arr_realloc(pos+1);

            return result;
        }

        // Find separator inside quotas and replace it with temporary character
        const char *replace_to_str = "_$#%*@!-+|\001";
        char replace_ch = find_replace_to_char(string, replace_to_str);
        int replaced = replace_char_between_quotas((char*)string, ' ', replace_ch);

        int leftBound = 0;
        int rightBound = 0;
        int length = strlen(string);
        int sep_count = strlen(separators);

        int i;
        int parts_count = 0;

        while(rightBound != length) {
            for(i = 0; i < sep_count; i++) {
                if(string[rightBound] == separators[i]) {
                    if(with_empty || rightBound >= leftBound) {
                      int new_str_len = rightBound - leftBound;

                      arr_inc_size(pos + 1);
                      result[pos++] = malloc(new_str_len + 1);
                      memcpy(result[pos - 1], string + leftBound, new_str_len);
                      result[pos - 1][new_str_len] = '\0';

                      leftBound = rightBound + 1;
                      parts_count++;
                      break;
                    }
                }
            }

            rightBound++;

            if(parts_count >= max_parts - 1 && max_parts != 0) {
                break;
            }
        }

        // Add one empty line at the end of array
        if(with_empty || leftBound < length) {
            int new_str_len = length - leftBound;

            arr_inc_size(pos + 1);
            result[pos++] = malloc(new_str_len + 1);
            memcpy(result[pos - 1], string + leftBound, new_str_len);

            result[pos - 1][new_str_len] = '\0';
        }

        result[pos] = NULL;
        arr_realloc(pos+1);

        if(replaced) {

            int i;
            replace_char_between_quotas((char*)string, replace_ch, ' ');

            for(i = 0; i <= pos; i++) {
                if(result[i] != NULL)
                    replace_char_between_quotas(result[i], replace_ch, ' ');
            }
        }

        return result;
    }

    return NULL;
}

/**
 * Combine string array to string
 *
 * @param arr String array
 * @param separator
 * @return Combined string, should be free after use
 */
char *ksnet_stringArrCombine(ksnet_stringArr arr, const char* separator) {

    if(arr == NULL) return NULL;

    int i, len = 0, len_separator = strlen(separator);

    // Define string length    
    for(i = 0; arr[i] != NULL; i++) {
        len += strlen(arr[i]) + len_separator + 1;
    }
    if(!len) return NULL;

    // Create string
    char *str = malloc(len);
    str[0] = '\0';
    for(i = 0; arr[i] != NULL; i++) {
        if(i) strcat(str, separator);
        strcat(str, arr[i]);
    }

    return str;
}

/**
 * Remove extra chars (double chars) from string
 *
 * Remove double characters from extras array in str array.
 * For example: if str = " --arg = 01" and extras = " =",
 * the result will be: --arg 01
 *
 * @param str
 * @param extras
 * @return The same string with extras removed
 */
static char *remove_extras(char *str, const char *extras) {

    int i, j = 0, k = 0, flg = 0, ex = 0;

    for(j = 0; str[j] != '\0'; j++) {

        ex = 0;
        for(i = 0; extras[i] != '\0'; i++) {
            if(str[j] == extras[i]) {
                ex = 1;
                break;
            }
        }

        if(ex) {
            if(!flg) {
                str[k++] = str[j];
                flg++;
            }
        }

        else {
            str[k++] = str[j];
            flg = 0;
        }
    }
    str[k] = 0;

    return str;
}

/**
 * Replace character between quotas
 *
 * @param str
 * @param replace_from
 * @param replace_to
 * @return
 */
static int replace_char_between_quotas(char *str, char replace_from, char replace_to) {

    int i, beg = 0,
        replaced = 0,
        quotesBeg = -1,
        quotesEnd = -1;

    for(;;) {

        for(i = beg; str[i] != 0; i++) {

            if(str[i] == '\"') {
                if(quotesBeg >= 0) {
                    quotesEnd = i;
                    break;
                }
                else quotesBeg = i;
            }
        }

        if(quotesBeg >= 0 && quotesEnd > 0) {

            for(i = quotesBeg+1; i < quotesEnd; i++) {
                if(str[i] == replace_from) {
                    str[i] = replace_to;
                    replaced = 1;
                }
            }

            beg = quotesEnd + 1;
            quotesBeg = -1;
            quotesEnd = -1;
        }
        else break;
    }

    return replaced;
}

/**
 * Find character which absent in string
 *
 * @param str
 * @param replace_to_str
 * @return
 */
static char find_replace_to_char(const char *str, const char *replace_to_str) {

    int i, j;
    char replace_to_char = 0;

    for(j = 0; replace_to_str[j] != 0; j++) {
        for(i = 0; str[i] != 0; i++) {
            if(replace_to_str[j] == str[i]) break;
        }
        if(str[i] == 0) {
            replace_to_char = replace_to_str[j];
            break;
        }
    }

    return replace_to_char;
}
