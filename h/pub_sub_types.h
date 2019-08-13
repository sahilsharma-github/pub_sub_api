#ifndef PUB_SUB_TYPES_H
#define PUB_SUB_TYPES_H


#include "pub_sub_defs.h"
#include<pthread.h>
//typedef enum  { LOGICAL_ID, CONTEXT, TAG, VALUE } Fields;
typedef enum  { FAILURE = -1, SUCCESS = 0} Result;
typedef enum { INTEGER = 1, FLOAT, STRING, CONTEXT_STRING } tlv_type;

//typedef void (*callback_func) (void);
typedef pthread_t Thread;

typedef struct
{
    char logical_id[PUB_SUB_MAX_STR_LEN];
    char tag[PUB_SUB_MAX_STR_LEN];
//  char context[PUB_SUB_MAX_STR_LEN];
    char value[PUB_SUB_MAX_STR_LEN];
} s_notification_info;

typedef void (*pub_sub_callback) (char * lid, char * tag, char * value);
//typedef void (*pub_sub_callback) (s_notification_info);

typedef struct
{
    Byte ie_type;
    uint_16 ie_length;
    char ie_value[PUB_SUB_MAX_STR_LEN];
}s_tlv;

#endif
