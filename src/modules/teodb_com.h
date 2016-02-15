/** 
 * \file   teodb_com.h
 * \author kirill
 *
 * Created on February 15, 2016, 2:44 AM
 */

#ifndef TEODB_COM_H
#define TEODB_COM_H

#include <stdint.h>

/**
 * Teonet database API commands
 */
enum CMD_D {

                        ///< The TYPE_OF_REQUEST field: "JSON:" or empty
                        ///< The ID field:  id of reques, it resend to user in answer
    CMD_D_SET = 129,    ///< #129 Set data request:  TYPE_OF_REQUEST: { namespace, key, data, data_len } }
    CMD_D_GET,          ///< #130 Get data request:  TYPE_OF_REQUEST: { namespace, key, ID } }
    CMD_D_LIST,         ///< #131 List request:  TYPE_OF_REQUEST: { ID, namespace } }
    CMD_D_GET_ANSWER,   ///< #132 Get data response: { namespace, key, data, data_len, ID } }
    CMD_D_LIST_ANSWER,  ///< #133 List response:  [ key, key, ... ]
    
    // Reserved
    CMD_R_NONE          ///< Reserved
};

/**
 * Teo DB binary network structure
 */
typedef struct teo_db_data {
    
    uint8_t key_length; ///< Key length
    uint32_t data_length; ///< Data length
    uint32_t id; ///< Request ID
    char key_data[]; ///< Key and Value buffer     
    
} teo_db_data;

#ifdef __cplusplus
extern "C" {
#endif




#ifdef __cplusplus
}
#endif

#endif /* TEODB_COM_H */

