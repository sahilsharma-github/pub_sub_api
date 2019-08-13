#ifndef PUB_SUB_MSG_DEFINES_H
#define PUB_SUB_MSG_DEFINES_H

#include "pub_sub_defs.h"
#include "pub_sub_types.h"

extern const int MESSAGE_SIZE_START_POSITION;
extern const int MESSAGE_TYPE_START_POSITION;
extern const int MESSAGE_SUB_TYPE_START_POSITION;
extern const int NUMBER_TLV_START_POSITION;
extern const int MESSAGE_BODY_START_POSITION;

// Message Types
extern const Byte CONTROL;
extern const Byte DATA;

// DATA Message Sub Types
extern const Byte PUBLISH_DATA;
extern const Byte NOTIFY_DATA;
extern const Byte PUBLISH_LID_LIST;
extern const Byte PUBLISH_TAG_LIST;
extern const Byte PUBLISH_CONTEXT_LIST;

// CONTROL Message Sub Types
extern const Byte SUBSCRIBE_REQUEST;
extern const Byte REGISTER_DATA;
extern const Byte RETRIEVE_LOGICAL_ID_LIST;
extern const Byte RETRIEVE_TAG_LIST;
extern const Byte RETRIEVE_CONTEXT_LIST;


typedef struct
{
    uint_16 msg_size;
    Byte message_type;
    Byte message_sub_type;
    uint_16 n_tlv;
}s_header;

typedef struct
{
    s_header hdr;
	s_tlv fields[4];
}generic_msg;

#endif