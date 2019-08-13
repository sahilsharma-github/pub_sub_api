#ifndef PUB_SUB_API_H
#define PUB_SUB_API_H


#include "pub_sub_types.h"
#include "pub_sub_msg_defines.h"
#include "../h/pub_sub_transcoder.h"
#include "../utilities/h/pub_sub_hash_container.h"
#include "../utilities/h/pub_sub_list.h"


//Publisher API
Result init_publish_api(const char * if_name);
void publish_info(const char * lid, const char * tag, const char * val);
void publish_cinfo(const char * lid, const char * tag, char * context, const char * val);

//Register API
void register_lid(const char * logical_id);
void register_lid_tag(const char * logical_id, const char * value_tag);
void register_lid_tag_context(const char * logical_id, char * value_tag, const char * context);

// Subscriber API
void subscribe_tag( pub_sub_callback cb, char * value_tag);
void subscribe_context( pub_sub_callback cb, char * ctxt);
void subscribe_lid_tag( pub_sub_callback cb, char * logical_id, char * value_tag);

#endif
