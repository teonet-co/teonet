/** 
 * \file   teodb_com.h
 * \author kirill
 *
 * Created on February 15, 2016, 3:43 PM
 */

#include <stdlib.h>
#include <string.h>

#include "teodb_com.h"

teo_db_data *prepare_request_data(const void *key, size_t key_len, 
        const void *data, size_t data_len, uint32_t id, size_t *tdd_len) {
    
    // Prepare data
    if(data == NULL) data_len = 0;
    *tdd_len = sizeof(teo_db_data) + key_len + data_len;
    teo_db_data *tdd = malloc(*tdd_len);
    tdd->key_length = key_len;
    tdd->data_length = data_len;
    tdd->id = id;
    memcpy(tdd->key_data, key, key_len);
    if(data != NULL && data_len)
        memcpy(tdd->key_data + key_len, data, data_len);

    return tdd;
}
        
        