#ifndef FILTER_APP_H
#define FILTER_APP_H

#include "../../h/pub_sub_types.h"

Result create_and_bind();
void notify_subscriber(generic_msg * msg, int fd);
void process_pub_data_msg(generic_msg * msg);
void process_subscribe_req_msg(generic_msg * msg, int fd);
Result process_connections( );

#endif
