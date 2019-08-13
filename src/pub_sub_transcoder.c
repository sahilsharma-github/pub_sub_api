#include "../h/pub_sub_transcoder.h"
#include<string.h>
#include<stdlib.h>
#include<stdio.h>

/*char * encode(int * p_size, generic_msg msg)
{
}*/
//char * encode1(int * p_size, generic_msg * msg)
char * encode(int * p_size, generic_msg * msg)
{
	size_t sz = sizeof(msg->hdr);
	for( int i = 0 ; i < msg->hdr.n_tlv ; ++i)
	{
		sz += sizeof(msg->fields[i].ie_type);
		sz += sizeof(msg->fields[i].ie_length);
		sz += msg->fields[i].ie_length;
		//sz += strlen(msg->fields[i].ie_value)+1;
//        printf("In transcoder: %d\n",  msg->fields[i].ie_length);
	}
	char * buf = (char*)malloc(sz);
	*p_size = sz;

    *((uint_16*)&buf[MESSAGE_SIZE_START_POSITION]) = sz;
	buf[MESSAGE_TYPE_START_POSITION] = msg->hdr.message_type;	
	buf[MESSAGE_SUB_TYPE_START_POSITION] = msg->hdr.message_sub_type;
	*((uint_16*)&buf[NUMBER_TLV_START_POSITION]) = msg->hdr.n_tlv;
	int index = MESSAGE_BODY_START_POSITION;
	for( int i = 0 ; i < msg->hdr.n_tlv ; ++i)
	{
		buf[index] = msg->fields[i].ie_type;
		++index;
		*((uint_16*)&buf[index]) = msg->fields[i].ie_length;
		++index; ++index;
		int j;
  //      printf("msg->fields[i].ie_length: %d\n", msg->fields[i].ie_length);
		//for( j = 0 ; j < strlen(msg->fields[i].ie_value) ; ++j,++index)
		for( j = 0 ; j < msg->fields[i].ie_length ; ++j,++index)
			buf[index] = msg->fields[i].ie_value[j];
		//buf[index] = '\0';
		//++index;
	}
	return buf;
}

uint_16 get_msg_size( char * buf, int index)
{
//    printf("index in get_msg_size : %d\n", index);
    uint_16 sz = *((uint_16*)&buf[MESSAGE_SIZE_START_POSITION+index]);
    return sz;
}

void free_msg(generic_msg * msg)
{
    for( int i = 0 ; i < msg->hdr.n_tlv ; ++i)
    {
        free(&msg->fields[i]);
    }
    free(msg);
  
}
/*generic_msg decode(char * buf, int i_len)
{
}*/
//void decode1(char * buf, generic_msg * msg, int i_len)
void decode(char * buf, generic_msg * msg, int i_len)
{
	//generic_msg * msg = malloc(sizeof(generic_msg));
    //printf("index in decode : %d\n", i_len);
    msg->hdr.msg_size = *((uint_16*)&buf[MESSAGE_SIZE_START_POSITION+i_len]);
	msg->hdr.message_type = buf[MESSAGE_TYPE_START_POSITION+i_len];
	msg->hdr.message_sub_type = buf[MESSAGE_SUB_TYPE_START_POSITION+i_len];
	msg->hdr.n_tlv = *((uint_16 *)&buf[NUMBER_TLV_START_POSITION+i_len]);
	int index = MESSAGE_BODY_START_POSITION+i_len;
    int j;

    //s_tlv * data;
    //printf("sg->hdr.n_tlv: %d\n", msg->hdr.n_tlv);
    for( int i = 0 ; i < msg->hdr.n_tlv ; ++i)
	{
		//s_tlv data;
        //Byte type = buf[index]; 
        //data = & msg->fields[i];
		//data->ie_type = buf[index];	
		msg->fields[i].ie_type = buf[index];	
		++index;
        //uint_16 len = *((uint_16 *)&buf[index]);
		//data->ie_length =  *((uint_16 *)&buf[index]);
		msg->fields[i].ie_length =  *((uint_16 *)&buf[index]);
		index += 2;
        //char * value = malloc(len);
        /*size_t sz = 3+len;
        data = malloc(sz);
        data->ie_type = type;
        data->ie_length = len;
        */
//        int j;
        //for( j = 0 ; j < msg->fields[i].ie_length ; ++j,++index)
    //    printf("msg->fields[i].ie_length in decode is %d\n", msg->fields[i].ie_length);
        for( j = 0 ; j < msg->fields[i].ie_length ; ++j,++index)
			msg->fields[i].ie_value[j] = buf[index];
	    //msg->fields[i] = *data;
    }
	//return msg;
}
