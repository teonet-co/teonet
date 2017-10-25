/** 
 * \file   teodb_com.h
 * \author kirill
 *
 * Created on February 15, 2016, 2:44 AM
 */

#ifndef TEODB_COM_H
#define TEODB_COM_H

#include <stddef.h>
#include <stdint.h>

/**
 * Teonet database API commands
 */
enum CMD_D {

                                ///< The TYPE_OF_REQUEST field: "JSON:" or empty
                                ///< The ID field:  id of reques, it resend to user in answer
    CMD_D_SET = 129,            ///< #129 Set data request:  TYPE_OF_REQUEST: { namespace, key, data, data_len } }
    CMD_D_GET,                  ///< #130 Get data request:  TYPE_OF_REQUEST: { namespace, key, ID } }
    CMD_D_LIST,                 ///< #131 List request:  TYPE_OF_REQUEST: { key, ID, namespace } }
    CMD_D_GET_ANSWER,           ///< #132 Get data response: { namespace, key, data, data_len, ID } }
    CMD_D_LIST_ANSWER,          ///< #133 List response:  [ key, key, ... ]
    
    CMD_D_LIST_LENGTH,          ///< #134 List length request:  TYPE_OF_REQUEST: { ID, namespace } }
    CMD_D_LIST_LENGTH_ANSWER,   ///< #135 List response:  { listLength, key, ID }
    
    CMD_D_LIST_RANGE,           ///< #136 List length request:  TYPE_OF_REQUEST: { ID, namespace } }
    CMD_D_LIST_RANGE_ANSWER,    ///< #137 List response:  { listLength, key, ID }
    
    CMD_D_ERROR_ANSWER,         ///< #138 Bad request answer
    
    // Reserved
    CMD_R_NONE                  ///< Reserved
};

#pragma pack(push)
#pragma pack(1)

/**
 * Teo DB binary network structure
 */
typedef struct teo_db_data {
    
    uint8_t key_length; ///< Key length
    uint32_t data_length; ///< Data length
    uint32_t id; ///< Request ID
    char key_data[]; ///< Key and Value buffer     
    
} teo_db_data;

/**
 * CMD_D_GET_LIST_RANGE extended Data structure
 */
typedef struct teo_db_data_range {
    
    uint32_t from; ///< From index (begin from zero)
    uint32_t to; ///< To index (not include))
    
} teo_db_data_range;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

teo_db_data *prepare_request_data(const void *key, size_t key_len, 
        const void *data, size_t data_len, uint32_t id, size_t *tdd_len);


#ifdef __cplusplus
}
#endif

#endif /* TEODB_COM_H */

