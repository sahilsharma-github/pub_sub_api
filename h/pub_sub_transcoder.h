#ifndef PUB_SUB_TRANSCODER_H
#define PUB_SUB_TRANSCODER_H

#include "pub_sub_msg_defines.h"
char * encode(int * p_size, generic_msg * msg);
//char * encode(int * p_size, generic_msg msg);
void decode(char * buf, generic_msg * msg, int i_len);
//generic_msg  decode(char * buf, int index);
uint_16 get_msg_size( char * buf, int index);

#endif
