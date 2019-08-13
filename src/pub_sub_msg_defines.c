#include "../h/pub_sub_msg_defines.h"

const int MESSAGE_SIZE_START_POSITION = 0;
const int MESSAGE_TYPE_START_POSITION = 2;
const int MESSAGE_SUB_TYPE_START_POSITION = 3;
const int NUMBER_TLV_START_POSITION = 4;
const int MESSAGE_BODY_START_POSITION = 6;

const Byte CONTROL = (Byte)1;
const Byte DATA = (Byte)2;

const Byte PUBLISH_DATA = (Byte)1;
const Byte NOTIFY_DATA = (Byte)2;
const Byte PUBLISH_LID_LIST = (Byte)3;
const Byte PUBLISH_TAG_LIST = (Byte)4;
const Byte PUBLISH_CONTEXT_LIST = (Byte)5;

const Byte SUBSCRIBE_REQUEST = (Byte)1;
const Byte REGISTER_DATA = (Byte)2;
const Byte RETRIEVE_LOGICAL_ID_LIST = (Byte)3;
const Byte RETRIEVE_TAG_LIST = (Byte)4;
const Byte RETRIEVE_CONTEXT_LIST = (Byte)5;
