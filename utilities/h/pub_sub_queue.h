#ifndef PUB_SUB_QUEUE_H
#define PUB_SUB_QUEUE_H

#include "pub_sub_util_defs.h"
#include "pub_sub_util_types.h"

void push(generic_msg msg, pub_sub_queue * q);
generic_msg* pop(pub_sub_queue * q);

void push_msg(char * buf, pub_sub_msg_queue * q);
char * pop_msg(pub_sub_msg_queue * q);

#endif
