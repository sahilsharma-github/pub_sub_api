#include "../../utilities/h/pub_sub_util_types.h"
#include "../../utilities/h/pub_sub_hash_container.h"
#include "../../utilities/h/pub_sub_list.h"
#include "../../utilities/h/pub_sub_queue.h"
#include "../../h/pub_sub_msg_defines.h"
#include "../../h/pub_sub_transcoder.h"
#include "filter_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<time.h>
#include<pthread.h>
pub_sub_string_map lidtag_map;
pub_sub_string_map tag_map;
pub_sub_string_map context_map;

//Thread job_processor;
pthread_cond_t pub_sub_cv = PTHREAD_COND_INITIALIZER;
pub_sub_generic_queue msg_jobs;
pthread_mutex_t pub_sub_mutex = PTHREAD_MUTEX_INITIALIZER;

static int q_elems = 0;
static long cur_line_start_pos = 0;
static int tag_offset = 28;
static int context_offset = 56;
static int pubfd;
static int regfd;
static int subfd;
static char file_path[PUB_SUB_MAX_STR_LEN];
static char msg_processing_mode[PUB_SUB_MAX_STR_LEN]; 
static struct sockaddr_in regaddr;
static struct sockaddr_in pubaddr;
static struct sockaddr_in subaddr;
static struct sockaddr_in clientaddr;
static pub_sub_list lids;
static pub_sub_list tags;
static pub_sub_list contexts;

Result create_and_bind()
{
	subfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	pubfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	regfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
	if ( pubfd < 0 || subfd < 0 || regfd < 0) 
	{
		perror("Failed to create server sockets:\n");
   		//exit(1);
		return FAILURE;
	}
	int optval = 1, ret_val;
    //int recvbuf_size = 1024*1024*10;
    int recvbuf_size = 16777216;                // max udp socket recv buffer size
	ret_val = setsockopt(pubfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
	if( ret_val < 0)
    {
        perror("setsockopt error while setting broadcast options:\n");
        return FAILURE;
    }
	
    ret_val = setsockopt(regfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
	if( ret_val < 0)
    {
        perror("setsockopt error while setting register broadcast options:\n");
        return FAILURE;
    }
	ret_val = setsockopt(regfd, SOL_SOCKET, SO_RCVBUF, &recvbuf_size, sizeof(recvbuf_size));
    
    if( ret_val < 0)
    {
        perror("setsockopt error while setting recv buffer:\n");
        return FAILURE;
    }

	ret_val = setsockopt(pubfd, SOL_SOCKET, SO_RCVBUF, &recvbuf_size, sizeof(recvbuf_size));
    if( ret_val < 0)
    {
        perror("setsockopt error while setting recv buffer:\n");
        return FAILURE;
    }

    ret_val = setsockopt(subfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if( ret_val < 0)
    {
        perror("setsockopt error while setting REUSE options:\n");
        return FAILURE;
    }
	memset(&regaddr, 0, sizeof(regaddr));		// make sure doesn't happen twice
	regaddr.sin_family = AF_INET;
	regaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    regaddr.sin_port = htons(REG_BROADCAST_PORT);

	memset(&pubaddr, 0, sizeof(pubaddr));		// make sure doesn't happen twice
	pubaddr.sin_family = AF_INET;
	pubaddr.sin_addr.s_addr = htonl(INADDR_ANY);;
	//pubaddr.sin_addr.s_addr = INADDR_ANY;
	//serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    pubaddr.sin_port = htons(PUB_BROADCAST_PORT);

	memset(&subaddr, 0, sizeof(subaddr));		// make sure doesn't happen twice
	subaddr.sin_family = AF_INET;
	subaddr.sin_addr.s_addr = INADDR_ANY;
    subaddr.sin_port = htons(SUB_FILTER_PORT);
	ret_val = bind(pubfd, (struct sockaddr *) &pubaddr, sizeof(pubaddr));
	if( ret_val < 0 ) 
	{
		perror("Failed to bind server sockets to receive broadcast:\n");
		return FAILURE;
	}
    
	ret_val = bind(regfd, (struct sockaddr *) &regaddr, sizeof(regaddr));
	if( ret_val < 0 ) 
	{
		perror("Failed to bind server sockets to receive register broadcast:\n");
		return FAILURE;
	}
	ret_val = bind(subfd, (struct sockaddr *) &subaddr, sizeof(subaddr));
	if( ret_val < 0 ) 
	{
		perror("Failed to bind server sockets to receive subscriber request:\n");
		return FAILURE;
	}
	return SUCCESS;
}

void notify_subscriber(generic_msg * msg, int fd)
{
    static int count = 1;
    msg->hdr.message_sub_type = NOTIFY_DATA;
    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    int ret_val = send(fd, buf, *ptr, 0);
    if( ret_val < 0 )
    {
        perror("Error while sending notify data to client active on fd: %d\n");
        printf("fd: %d\n", fd);
        close(fd);
    }
    else
    {
        printf("Client active on fd %d was notified with message size %d. Now count is %d\n", fd, *ptr, count);
        //if(count > 99990)
        //printf("current send count is %d\n", count);
        ++count;
    }
    free(buf);
}
void process_register_msg(generic_msg * msg)
{
    printf("process_register_msg\n");
    FILE * fp = fopen(file_path, "a");
    char str[1024] = "";
    if( !fp )
    {
        perror("Error while modifying the data file. Program will exit\n");
        exit(1);
    }
    cur_line_start_pos = ftell(fp);

    if( msg->hdr.n_tlv == 1)
    {
     
        if( pub_sub_list_push(&lids, msg->fields[0].ie_value, strlen(msg->fields[0].ie_value)+1) == SUCCESS)
        {
            printf("Updated lid list with %s and size %d\n", msg->fields[0].ie_value, strlen(msg->fields[0].ie_value)+1);
            fprintf(fp, "%s ||    ||", msg->fields[0].ie_value); 
            //sprintf(str, "%s", msg->fields[0].ie_value);
        }
        
    }
    else if( msg->hdr.n_tlv == 2)
    {
        if(pub_sub_list_push(&lids, msg->fields[0].ie_value, strlen(msg->fields[0].ie_value)+1) == SUCCESS)
        {
            printf("Updated lid list with %s and size %d\n", msg->fields[0].ie_value, strlen(msg->fields[0].ie_value)+1);
            fprintf(fp, "%s || ", msg->fields[0].ie_value); 
            sprintf(str, msg->fields[0].ie_value);
        }
        
        if( pub_sub_list_push(&tags, msg->fields[1].ie_value, strlen(msg->fields[1].ie_value)+1) == SUCCESS)
        {
            printf("Updated tag list with %s and size %d\n", msg->fields[1].ie_value, strlen(msg->fields[1].ie_value)+1);
            //fseek(fp, tag_offset, cur_line_start_pos);
            //fprintf(fp, "%s\n", msg->fields[1].ie_value);
            
            if( !strlen(str))
                fprintf(fp, "    || %s ||", msg->fields[1].ie_value);
                //sprintf(str, "\t\t%-10s", msg->fields[1].ie_value);
            else
            {
                fprintf(fp, "%s ||", msg->fields[1].ie_value);
                //strcat(str, "\t\t");
                //strcat(str, msg->fields[1].ie_value);
                //strcat(str, "\n");
            }
        }
        else
        {
            if( !strlen(str))
                 fprintf(fp, "    ||");
        }
    }
    else if( msg->hdr.n_tlv == 3)
    {
        bool tag_present = false;
        if(pub_sub_list_push(&lids, msg->fields[0].ie_value, strlen(msg->fields[0].ie_value)+1) == SUCCESS)
        {
            printf("Updated lid list with %s and size %d\n", msg->fields[0].ie_value, strlen(msg->fields[0].ie_value)+1);
            fprintf(fp, "%s || ", msg->fields[0].ie_value);
            sprintf(str, msg->fields[0].ie_value);
        }

        if( pub_sub_list_push(&tags, msg->fields[1].ie_value, strlen(msg->fields[1].ie_value)+1) == SUCCESS)
        {
            tag_present = true;
            printf("Updated tag list with %s and size %d\n", msg->fields[1].ie_value, strlen(msg->fields[1].ie_value)+1);
            //fseek(fp, tag_offset, cur_line_start_pos);
            //fprintf(fp, "%s", msg->fields[1].ie_value);
            if( !strlen(str))
            {
                fprintf(fp, "    || %s || ", msg->fields[1].ie_value);
                sprintf(str, "\t\t\t\t\t\t\t\t%-10s", msg->fields[1].ie_value);
            }
            else
            {
                fprintf(fp, "%s || ", msg->fields[1].ie_value);
                //strcat(str, "\t\t\t\t\t\t");
                //strcat(str, msg->fields[1].ie_value);
                //strcat(str, "\n");
            }
        }

        if(pub_sub_list_push(&contexts, msg->fields[2].ie_value, strlen(msg->fields[2].ie_value)+1) == SUCCESS)
        {
            printf("Updated ctxt list with %s and size\n", msg->fields[2].ie_value, strlen(msg->fields[2].ie_value)+1);
            //fseek(fp, context_offset, cur_line_start_pos);
            //fprintf(fp, "%-10s\n", msg->fields[2].ie_value);

            if(!strlen(str))
            {
                fprintf(fp, "    ||    || ", msg->fields[2].ie_value);
                sprintf(str, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t%-10s", msg->fields[2].ie_value);
            }
            else
            {
                if( tag_present)
                    fprintf(fp, " %s", msg->fields[2].ie_value);
                else
                    fprintf(fp, "    || %s", msg->fields[2].ie_value);
                //strcat(str, "\t\t\t\t\t\t\t");
                //strcat(str, msg->fields[2].ie_value);
                //strcat(str, "\n");
            } 
        }
        else
        {
             if( !strlen(str))
                 fprintf(fp, "    ||");

        }
    }
    fprintf(fp, "\n");
    fflush(fp);
    fclose(fp);
}
void process_pub_data_msg(generic_msg * msg)
{
    printf("process_pub_data_msg:\n");
    //char keys[2][PUB_SUB_MAX_STR_LEN];
    //char ** keys = malloc(sizeof(char*) * 2);
    char * keys[2];
    pub_sub_list list1;
    pub_sub_list list2;
    pub_sub_list list3;
    pub_sub_list_init(&list1);
    pub_sub_list_init(&list2);
    pub_sub_list_init(&list3);
    if( msg->hdr.n_tlv < 3)
        printf("Publish Data message receievd with insufficient data. DIscarding!\n");
    else if( msg->hdr.n_tlv == 3)
    {
        //strcpy( keys[0], msg->fields[0].ie_value);
        //strcpy( keys[1], msg->fields[1].ie_value);
        keys[0] = msg->fields[0].ie_value;
        keys[1] = msg->fields[1].ie_value;
        //printf("%s\n", keys[0]);
        //printf("%s\n",keys[1]);
        //printf("Here1\n");
        pub_sub_string_hash_search( &lidtag_map, 2, keys, &list1);
        //printf("list1 size: %d\n", get_pub_sub_list_size(&list1));
        //strcpy( keys[0], msg->fields[1].ie_value);
        keys[0] = msg->fields[1].ie_value;
        //printf("Here2\n");
        pub_sub_string_hash_search( &tag_map, 1, keys, &list2);
        //printf("Here3\n");
    }
    else if( msg->hdr.n_tlv == 4) 
    {
        /*strcpy( keys[0], msg->fields[0].ie_value);
        strcpy( keys[1], msg->fields[1].ie_value);
        strcpy( keys[2], msg->fields[2].ie_value);*/
        keys[0] = msg->fields[0].ie_value;
        keys[1] = msg->fields[1].ie_value;
        //keys[2] = msg->fields[2].ie_value;
        pub_sub_string_hash_search( &lidtag_map, 2, keys, &list1);
        //strcpy( keys[0], "");
        //strcpy( keys[0], msg->fields[1].ie_value);
        keys[0] = msg->fields[1].ie_value;
        pub_sub_string_hash_search( &tag_map, 1, keys, &list2);
        //strcpy( keys[0], msg->fields[2].ie_value);
        keys[0] = msg->fields[2].ie_value;
        pub_sub_string_hash_search( &context_map, 1, keys, &list3);
    }
    //printf("Here\n");
    if( get_pub_sub_list_size( &list1) == 0 && get_pub_sub_list_size( &list2) == 0 && get_pub_sub_list_size( &list3) == 0 )
    {
        printf("All maps are empty. No one subscribed for this data.\n");
        pub_sub_list_free(&list1);
        pub_sub_list_free(&list2);
        pub_sub_list_free(&list3);
        //free(keys);
        return;
    }
    
    int fd[PUB_SUB_MAX_BUCKETS] = {[0 ... 1023] = -1};                        // unique array for accept fd set
    pub_sub_generic_node * node = list1.head;
    while( node != NULL )
    {
        fd[atoi((char*)node->data)] = atoi((char*)node->data);
        node = node->next;
    }
    node = list2.head;
    while( node != NULL )
    {
        fd[atoi((char*)node->data)] = atoi((char*)node->data);
        node = node->next;
    }
    node = list3.head;
    while( node != NULL )
    {
        fd[atoi((char*)node->data)] = atoi((char*)node->data);
        node = node->next;
    }
    for(int i = 0 ; i < PUB_SUB_MAX_BUCKETS ; ++i)
    {
        if (fd[i] != -1 )         // forward the msg to active connections
        {
            //printf("Notifying subscriber\n");
            notify_subscriber(msg, i);
        }
    }
    pub_sub_list_free(&list1);
    pub_sub_list_free(&list2);
    pub_sub_list_free(&list3);
    //free(keys);
}
void process_subscribe_req_msg(generic_msg * msg, int fd)
{
    static int sub_count =0;
    printf("In process_subscribe_req_msg ...%d \n", ++sub_count);
    switch(msg->hdr.n_tlv)
    {
        char keys[2][PUB_SUB_MAX_STR_LEN];
        case 2:
            //printf("Creating keys\n");
            strcpy( keys[0], msg->fields[0].ie_value);
            strcpy( keys[1], msg->fields[1].ie_value);
            /*printf("Keyargs[0]: %s\n", keys[0]);
            printf("Keyargs[1]: %s\n", keys[1]);
            printf("Updating hashmap\n");*/
            pub_sub_string_hash_insert(&lidtag_map, fd, 2, keys);
            printf("lidtag_map updated\n");
            break;
        case 1:
            if( msg->fields[0].ie_type == STRING)
            {
                strcpy(keys[0], msg->fields[0].ie_value);
                pub_sub_string_hash_insert(&tag_map, fd, 1, keys);
                printf("tag_map updated\n");
            }
            if( msg->fields[0].ie_type == CONTEXT_STRING)
            {
                strcpy(keys[0], msg->fields[0].ie_value);
                pub_sub_string_hash_insert(&context_map, fd, 1, keys);
                printf("context_map updated\n");
            }
            break;
        default:
            printf("These many TLVs are not allowed currently\n");
            break;
    }
}
void process_retrieve_data_msg( generic_msg * msg )
{
    printf("In process_retrieve_data_msg ... \n");
    generic_msg newmsg;
    int len = 0;
    newmsg.hdr.message_type = DATA;
    newmsg.hdr.n_tlv = 1;
    s_tlv msg_field;
    msg_field.ie_type = STRING;
    if(msg->hdr.message_sub_type == RETRIEVE_LOGICAL_ID_LIST)
    {
        pub_sub_generic_node * node = lids.head; 
        /*if( node != NULL)
        {
            memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);
            len += strlen((char*)node->data)+1;
            node = node->next; 
        }*/
        newmsg.hdr.message_sub_type = PUBLISH_LID_LIST;
        while(node != NULL)
        {
            //printf("len is: %d\n", len);
            if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN-1)
                memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data));
                
            else                        // send multiple messages in case the list size > 1024
            {
                printf("len is: %d\n", len);
                msg_field.ie_value[len] = 6;            // Using ACK as an indicator for receiver that there will be more msgs
                msg_field.ie_length = len+2;
                msg_field.ie_value[++len] = '\0';            // Using ACK as an indicator for receiver that there will be more msgs
               

     
                newmsg.fields[0] = msg_field;
                int msg_size = -1;
                int * ptr = &msg_size;
                char * buf = encode(ptr, &newmsg);
                sendto(regfd, buf, *ptr, 0, (const struct sockaddr *) &clientaddr, sizeof clientaddr);
                printf("sent reg reply of size %d\n", *ptr);
                len = 0;
                msg_field.ie_length = len;
                strcpy(msg_field.ie_value, "");
                //if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN)
                memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);

            }
            len += strlen((char*)node->data);
            msg_field.ie_value[len] = ';';              // delimiter
            ++len;
            node = node->next;
        }
        /*
        if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN)
        {
            memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);
            len += strlen((char*)node->data)+1;
        }*/
    }
    else if(msg->hdr.message_sub_type == RETRIEVE_TAG_LIST)
    {
        pub_sub_generic_node * node = tags.head;
        newmsg.hdr.message_sub_type = PUBLISH_TAG_LIST;
        /*if( node != NULL)
        {
            memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);
            len += strlen((char*)node->data)+1;
            node = node->next;
        }*/
        while( node != NULL)
        {
            if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN-1)
                memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);
            else
            {
                msg_field.ie_length = len+2;
                msg_field.ie_value[len] = 6;
                msg_field.ie_value[len+1] = '\0';
                newmsg.fields[0] = msg_field;
                int msg_size = -1;
                int * ptr = &msg_size;
                char * buf = encode(ptr, &newmsg);
                sendto(regfd, buf, *ptr, 0, (const struct sockaddr *) &clientaddr, sizeof clientaddr);
                printf("sent tag reply of size %d\n", *ptr);
                len = 0;
                msg_field.ie_length = len;
                strcpy(msg_field.ie_value, "");
                //if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN)
                memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);

            }
            len += strlen((char*)node->data)+1;
            msg_field.ie_value[len] = '^';              // delimiter
            ++len;
            node = node->next;
        }
        /*
        if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN)
        {
            memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);
            len += strlen((char*)node->data)+1;
        }*/

    }
    else if(msg->hdr.message_sub_type == RETRIEVE_CONTEXT_LIST)
    {
        pub_sub_generic_node * node = contexts.head;
        newmsg.hdr.message_sub_type = PUBLISH_CONTEXT_LIST;
       /* 
        if( node != NULL)
        {
            memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);
            len += strlen((char*)node->data)+1;
            node = node->next;
        }*/
        while( node != NULL)
        {
            if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN-1)
                memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);
            else
            {
                msg_field.ie_length = len+2;
                msg_field.ie_value[len] = 6;
                msg_field.ie_value[len+1] = '\0';
                newmsg.fields[0] = msg_field;
                int msg_size = -1;
                int * ptr = &msg_size;
                char * buf = encode(ptr, &newmsg);
                sendto(regfd, buf, *ptr, 0, (const struct sockaddr *) &clientaddr, sizeof clientaddr);
                printf("sent reg  context reply of size %d\n", *ptr);
                len = 0;
                msg_field.ie_length = len;
                strcpy(msg_field.ie_value, "");
                //if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN)
                memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);

            }
            len += strlen((char*)node->data)+1;
            msg_field.ie_value[len] = '^';              // delimiter
            ++len;
            node = node->next;
        }
        /*
        if( len+strlen((char*)node->data)+1 < PUB_SUB_MAX_STR_LEN)
        {
            memcpy(msg_field.ie_value+len, node->data, strlen((char*)node->data)+1);
            len += strlen((char*)node->data)+1;
        }*/

    }
    /*printf("len is %d\n", len);
        printf("value is:\n");
    for(int i = 0 ; i < len ; ++i)
        printf("%c", msg_field.ie_value[i]);
    printf("\n");*/
    msg_field.ie_value[len] = '\0';
    msg_field.ie_length = ++len;
    newmsg.fields[0] = msg_field;
    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, &newmsg);
    sendto(regfd, buf, *ptr, 0, (const struct sockaddr *) &clientaddr, sizeof clientaddr);
    printf("sent reg reply of size %d\n", *ptr); 
    free(buf);
}
void process_msg_on_register_fd(char * buf)
{
    generic_msg * msg = malloc(sizeof(generic_msg));
    decode(buf, msg, 0);
    if( msg->hdr.message_type == CONTROL )
    {
        if( msg->hdr.message_sub_type == REGISTER_DATA )
        {
            process_register_msg(msg);
        }
        else if( msg->hdr.message_sub_type == RETRIEVE_LOGICAL_ID_LIST || msg->hdr.message_sub_type == RETRIEVE_TAG_LIST 
                    || msg->hdr.message_sub_type == RETRIEVE_CONTEXT_LIST )
        {
            process_retrieve_data_msg(msg);
            printf("get all msg received\n");
        }
        else
            printf("Unknown CONTROL msg received\n");

    }
    else if( msg->hdr.message_type == DATA )
        printf("DATA msg received. Potentially wrong type. Discarding.\n");
    else
        printf("Unknow msg received\n");

   //printf("Reg message received\n");

}
    

void * msg_jobs_handler(void * data)
{
    clock_t start, end;
    double thr_avg = 0.0, thr_sum = 0.0; int thr_count = 0;
    generic_msg * msg = malloc(sizeof(generic_msg));
    while(true)
    {
        start = clock();
        pthread_mutex_lock(&pub_sub_mutex);
        if( q_elems < 1)
            pthread_cond_wait(&pub_sub_cv, &pub_sub_mutex); 
        pub_sub_queue_node * msg_node = q_pop(&msg_jobs);
        if( msg_node != NULL)
            --q_elems;
        /*if( q_elems < 1000)
            pthread_cond_signal(&pub_sub_cv);*/
        pthread_mutex_unlock(&pub_sub_mutex);
        if( msg_node != NULL )
        {
            decode((char*)msg_node->data, msg, 0);
            if( msg->hdr.message_type == DATA )
            {
                if( msg->hdr.message_sub_type == PUBLISH_DATA )
                {
                    process_pub_data_msg(msg);
                }
                else
                    printf("Unknow DATA msg received\n");
            }
            else if(msg->hdr.message_type == CONTROL)
            {
                if ( msg->hdr.message_sub_type == SUBSCRIBE_REQUEST)
                {
                    process_subscribe_req_msg(msg, msg_node->fd);
                }
                else
                    printf("Unknow CONTROL msg received\n");
            }
            else
                printf("Unknow msg received\n");
            free(msg_node->data);
            free(msg_node);
            end = clock();
            ++thr_count;
            //printf("thr_time: %f \n", ((double)(end-start))/CLOCKS_PER_SEC);
            thr_sum += ((double)(end-start))/CLOCKS_PER_SEC;
            thr_avg = thr_sum/thr_count;
             //printf("thr_avg: %f \n", thr_avg);
        }
    }
    free(msg);
}

Result process_connections( )
{
    printf("file_path is %s\n", file_path);
    printf("mode is %s\n", msg_processing_mode);
    fd_set master;
    fd_set read_fds;
    int maxfd;
	socklen_t addr_len = sizeof(clientaddr);
	int n_bytes;
	if (listen( subfd, PUB_SUB_INITIAL_BACKLOGS ) == -1)
	{
		perror("Server listen() failed:\n");
		return FAILURE;
	}
	FD_SET(subfd, &master);
	FD_SET(pubfd, &master);
    if( !strcmp(msg_processing_mode, "ON"))
    {
        FD_SET(regfd, &master);
        printf("Mode is on\n");
    }
	if( pubfd > subfd )
		maxfd = pubfd;
	else
		maxfd = subfd;
    if( !(strcmp(msg_processing_mode, "ON")) && maxfd < regfd)
        maxfd = regfd;
    char pub_msg[PUB_SUB_MAX_STR_LEN];
    char sub_msg[PUB_SUB_MAX_STR_LEN]; 
    char reg_msg[PUB_SUB_MAX_STR_LEN]; 
    struct timeval time_500ms = { 0, 500*1000 };
    int count = 0;
    generic_msg * msg = malloc(sizeof(generic_msg));
    clock_t start, end;
    clock_t start1, end1;
    clock_t start2, end2;
    struct timeval  tv1, tv2;
    int t_count = 0;
    double t_sum = 0.0;
    bool ismsg = false;
    double sel_avg = 0.0, sel_sum = 0.0; int sel_count = 0;
    double recv_avg = 0.0, recv_sum = 0.0; int recv_count = 0;
    double decod_avg = 0.0, decod_sum = 0.0; int decod_count = 0;
    double proc_pub_avg = 0.0, proc_pub_sum = 0.0; int proc_pub_count = 0;
	for(;;)
	{
        //struct timeval time_500ms = { 0, 500*1000 };
        //start = clock(); 
        //gettimeofday(&tv1, NULL);
		FD_ZERO(&read_fds);
		read_fds = master;
        //printf("\n\n-------------------------------------------------------------------------------------------\n");
		//printf("Performing select call on FD set with maxfd: %d\n", maxfd);
		//if( select(maxfd+1, &read_fds, NULL, NULL, NULL) < 0)
		//if( select(maxfd+1, &read_fds, NULL, NULL, &time_500ms) < 0)
		if( select(maxfd+1, &read_fds, NULL, NULL, NULL) < 0)
		{
			perror("Server Select() failed:\n");
			return FAILURE;
		}
        start = clock(); 
        /*end2 = clock();
        ++sel_count;
        printf("select time %f\n", ((double)(end2-start2))/CLOCKS_PER_SEC);
                        sel_sum += ((double)(end2-start2))/CLOCKS_PER_SEC;
        sel_avg = sel_sum/sel_count;
                        printf("select_avg time %f\n", sel_avg);
                        //printf("Msgs received are: %d\n", ++count);*/
		for( int i = 0 ; i <= maxfd; ++i)   
		{
			if(FD_ISSET(i, &read_fds))
			{
				if( i == pubfd)
				{
                    //start2 = clock(); 
                    //printf("Message received on Publisher FD\n");
					n_bytes = recvfrom(pubfd, pub_msg, sizeof(pub_msg), 0, (struct sockaddr *)&clientaddr, &addr_len);
                        //if(count>99990)
                        //printf("Msgs received are: %d\n", count);
                    //count++;
                    ismsg = true;
                    /*end = clock();
                    ++recv_count;
                    recv_sum += ((double)(end-start))/CLOCKS_PER_SEC;
                    printf("recv time %f\n", ((double)(end-start))/CLOCKS_PER_SEC);
                    recv_avg = recv_sum/recv_count;
                        printf("recv_avg time %f\n", recv_avg);
*/
					if( n_bytes > 0)
					{
                      /*  end2 = clock();
                        ++sel_count;
        printf("select time %f\n", ((double)(end2-start2))/CLOCKS_PER_SEC);
                        sel_sum += ((double)(end2-start2))/CLOCKS_PER_SEC;
        sel_avg = sel_sum/sel_count;
                        printf("select_avg time %f\n", sel_avg);*/

                        //static int count = 0;
                        //printf("Msgs received are: %d\n", ++count);
						//printf("Message received from Publisher with size: %d \n", n_bytes);
						//generic_msg * msg = decode(pub_msg, msg, 0);
                        //start = clock();
						//decode(pub_msg, msg, 0);
                        //start1 = clock();
                        pthread_mutex_lock(&pub_sub_mutex); 
                        //printf("q_elems while push: %d\n", q_elems);
                        //if( q_elems > 1000 )
                        //    pthread_cond_wait(&pub_sub_cv, &pub_sub_mutex);
                        q_push(pub_msg, &msg_jobs, i);
                        ++q_elems;
                        if( q_elems > 0 )
                            pthread_cond_signal(&pub_sub_cv);    
                        pthread_mutex_unlock(&pub_sub_mutex);
                        //end1 = clock();
                        //end = clock(); 
                        /*++decod_count;
                        printf("decod time %f\n", ((double)(end1-start1))/CLOCKS_PER_SEC);
                        decod_sum += ((double)(end1-start1))/CLOCKS_PER_SEC;
                        decod_avg = decod_sum/decod_count;
                        printf("decod_avg time %f\n", decod_avg);*/
                       /* if( msg->hdr.message_type == DATA )
                        {
                            if( msg->hdr.message_sub_type == PUBLISH_DATA )
                            {   
                         */       //start = clock();
                           //     process_pub_data_msg(msg);  
                                /*end = clock();
                                ++proc_pub_count; 
                                proc_pub_sum += ((double)(end-start))/CLOCKS_PER_SEC;
                                proc_pub_avg = proc_pub_sum/proc_pub_count;
                                printf("proc time %f\n", ((double)(end-start))/CLOCKS_PER_SEC);
                                printf("proc_avg time %f\n", proc_pub_avg);*/
                            /*}
                            else
                                printf("Unknow DATA msg received\n");
                        }
                        else
                            printf("Unknow msg received\n");*/
                       //free(msg);
					}
				}
                else if ( i == regfd )
                {
                    n_bytes = recvfrom(regfd, reg_msg, sizeof(reg_msg), 0, (struct sockaddr *)&clientaddr, &addr_len);
                    process_msg_on_register_fd(reg_msg);
                    /*
                    decode(reg_msg, msg, 0);
                    if( msg->hdr.message_type == CONTROL )
                    {
                        if( msg->hdr.message_sub_type == REGISTER_DATA )
                        {
                            process_register_msg(msg);
                        }
                        else
                            printf("Unknown CONTROL msg received\n");
                        
                    }
                    else if( msg->hdr.message_type == DATA )
                        printf("DATA msg received. Potentially wrong type. Discarding.\n");
                    else
                        printf("Unknow msg received\n");

                    printf("Reg message received\n");*/
                }
                else if ( i == subfd )
                {
					printf("Message received on Subscriber FD\n");
                    int newconn = accept( subfd, (struct sockaddr*)&clientaddr, &addr_len);
                    printf("Newfd is: %d\n", newconn);
                    if( newconn < 0 )
                        perror("Error while accepting connection: \n");
                    FD_SET( newconn, &master);
                    if( newconn > maxfd )
                        maxfd = newconn;
                    
                }
                else
                {
                    printf("Message received from a subscriber\n");
                    int index = 0, n_bytes = 0, recv_len = 0, bytes_in_buffer = 0;
                    uint_16 msg_size = 0;
                    n_bytes = recv(i, sub_msg, sizeof(sub_msg), 0);
                    bytes_in_buffer = n_bytes;
                    while( true )
                    {
                        printf("n_bytes from recv call is %d\n", n_bytes);
                        if( n_bytes < 0)
                        {
                            perror("Error while receving from subscriber application: \n"); 
                            close(i);
                            FD_CLR(i, &master);
                            return FAILURE;
                        }
                        else if( n_bytes == 0 )
                        {
                            perror("Connection closed by client: \n");
                            close(i);
                            FD_CLR(i, &master);
                            return FAILURE;
                        }
                        if( n_bytes != 1 )
                            msg_size = get_msg_size(sub_msg, index);
                        else
                        {
                            sub_msg[0] = sub_msg[index];
                            n_bytes += recv(i, sub_msg+1, (sizeof(sub_msg)-1), 0);
                            bytes_in_buffer = n_bytes;
                            if(n_bytes > 1)
                            {
                                index = 0;
                                recv_len = 0;
                                msg_size = get_msg_size(sub_msg, index);
                            }
                            printf("msg_size: %d\n", msg_size);
                        }

                        if( msg_size == n_bytes)      
                        {
                            //generic_msg * msg;
                            pthread_mutex_lock(&pub_sub_mutex);
                            q_push(sub_msg+index, &msg_jobs, i);
                            ++q_elems;
                            if( q_elems > 0 )
                                pthread_cond_signal(&pub_sub_cv);
                            pthread_mutex_unlock(&pub_sub_mutex);
                            break;
                            /*decode(sub_msg, msg, index);
                            if (msg->hdr.message_type == CONTROL)   
                            {
                                if ( msg->hdr.message_sub_type == SUBSCRIBE_REQUEST) // move this to a method
                                {
                                    process_subscribe_req_msg(*msg, i);
                                    break;
                                }
                                else
                                    printf("Message received from subscriber application with unknown msg sub type.\n");  
                            }
                            else
                                printf("Message received from subscriber application with unknown msg type.\n");  */
                        }
                        else if( msg_size > n_bytes )
                        {
                            for(int j = index, k = 0 ; j < bytes_in_buffer ; ++j, ++k)
                                sub_msg[k] = sub_msg[j];
                            index = 0;
    
                            recv_len = n_bytes;
                            n_bytes = recv(i, sub_msg+recv_len, sizeof(sub_msg)-recv_len, 0);
                            bytes_in_buffer = n_bytes+recv_len;
                            printf("n_bytes from next recv call is %d\n", n_bytes);
                            if( n_bytes == 0 )
                            {
                                printf("Connection closed\n");
                                close(i);   
                                return FAILURE;
                            }
                            n_bytes += recv_len;
                            continue;
                        }
                        else
                        {
                            //generic_msg * msg = decode(sub_msg, index);
                            //decode(sub_msg, msg, index);
                            //process_subscribe_req_msg(*msg, i);
                            pthread_mutex_lock(&pub_sub_mutex);
                            q_push(sub_msg+index, &msg_jobs, i);
                            ++q_elems;
                            if( q_elems > 0 )
                                pthread_cond_signal(&pub_sub_cv);
                            pthread_mutex_unlock(&pub_sub_mutex);

                            while( true )
                            {
                                index += msg_size;
                                n_bytes -= msg_size;
                                if( n_bytes != 1)
                                    msg_size = get_msg_size(sub_msg, index);
                                else
                                {
                                    printf("n_bytes is 1. \n");
                                    break;
                                }

                                printf("msg_size is: %d\n", msg_size);
                                printf("index is: %d\n", index);
                                printf("n_bytes is: %d\n", n_bytes);
                                if( msg_size <= n_bytes)
                                {
                                    //generic_msg * msg = decode(sub_msg, index);  
                                    pthread_mutex_lock(&pub_sub_mutex);
                                    q_push(sub_msg+index, &msg_jobs, i);
                                    ++q_elems;
                                    if( q_elems > 0 )
                                        pthread_cond_signal(&pub_sub_cv);
                                    pthread_mutex_unlock(&pub_sub_mutex);
                               //     decode(sub_msg, msg, index);   
                                    printf("pushed next msg\n");
                              //      process_subscribe_req_msg(*msg, i);
                                    if( msg_size ==  n_bytes)
                                        break;
                                    msg_size = get_msg_size(sub_msg, index);
                                }
                                else
                                    break;
                            }
                            if( msg_size ==  n_bytes)
                                break;
                        }
                    }

                }
			}
		}
        if(ismsg){
        ismsg = false;
        end = clock();
        //gettimeofday(&tv2, NULL);
        //++t_count;
        //t_sum += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
        /*printf ("Total time = %f seconds\n",
         (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec));*/
        //printf("Total time avg: %f\n", t_sum/t_count);
                    ++recv_count;
                    recv_sum += ((double)(end-start))/CLOCKS_PER_SEC;
          //          printf("recv time %f\n", (end-start)/(double)(CLOCKS_PER_SEC/1000));
                //    printf("recv time %f\n", ((double)(end-start))/CLOCKS_PER_SEC);
                    recv_avg = recv_sum/recv_count;
        //if (count > 99990){
        //printf("Total time avg: %f\n", t_sum/t_count);
                 //   printf("recv_avg time %f\n", recv_avg);
                 //   printf("recv_count is %d\n", recv_count);
        //}
        }

	}
    free(msg);
}
void pub_sub_strip( char * str)
{
    char * temp = malloc(strlen(str)+1);
    int i = 0, j = 0;
    while( str[i] != '\0' )
    {
        if( str[i] != ' ' && str[i] != '\n')
        {
            temp[j] = str[i];   
            ++j;
        }
        ++i;
    }
    temp[j] = '\0';
    strcpy(str, temp);
}	

int main()
{
    FILE * fp = fopen("filter_config", "r");
    char * path_tag = "FILE_PATH";
    char * mode_tag = "CONTROL_MESSAGE_PROCESSING_MODE";
    char str[PUB_SUB_MAX_STR_LEN];
    char * token, *val;
    if( !fp )
    {
        perror("Error while opening the filter_config file. Program will exit\n");
        exit(1);
    }
    while (fgets(str, PUB_SUB_MAX_STR_LEN, fp) != NULL)
    {
        token = strtok(str, "=");
        val = strtok(NULL, "=");
        pub_sub_strip(token);
        pub_sub_strip(val);
        if( !strcmp(token, path_tag))
        {   
            strcpy(file_path, val);
        }
        else if( !strcmp(token, mode_tag))
        {
            strcpy(msg_processing_mode, val);    
        }
    }

    fclose(fp);
  //  if( access(file_path, F_OK ) == -1)
  //  {
        printf("Creating file\n");
        fp = fopen(file_path, "w");
        if( !fp )
        {
            perror("Error while opening the data file. Program will exit\n");
            exit(1);
        }
        fprintf(fp, "Logical_IDs || ");
    //fseek(fp, tag_offset, SEEK_SET);
        fprintf(fp, "Tags || ");
    //fseek(fp, context_offset, SEEK_SET);
        fprintf(fp, "Contexts\n");
        fclose(fp);
   /* }
    else
    {
        printf("File exists. Updating local lists from file\n");
        fp = fopen(file_path, "a+");
        if( !fp )
        {
            perror("Error while opening the data file. Program will exit\n");
            exit(1);
        }
        char field[PUB_SUB_MAX_STR_LEN]; int i, j, count;
        fgets(str, PUB_SUB_MAX_STR_LEN, fp);
        while (fgets(str, PUB_SUB_MAX_STR_LEN, fp) != NULL)
        {   
            i = 0; j = 0, count = 0;
            while(str[i] != '\n')
            {
                //printf("str at %d is %d\n", i, str[i]);
                if( str[i] != 32 && str[i] != 124 )
                    field[j] = str[i];
                else
                {
                    field[j] = '\0';
                    printf("field is %s \n");
                    if(!j)
                    {
                        if( count == 0)
                        {
                            ++count;
                            if(pub_sub_list_push(&lids, field,  strlen(field)+1))
                                printf("Updated lid list with %s and size %d\n", field, strlen(field)+1);
                        }
                        else if( count == 1)
                        {
                            ++count;
                            if(pub_sub_list_push(&tags, field,  strlen(field)+1))
                                printf("Updated tags list with %s and size %d\n", field, strlen(field)+1);
                        }
                        else if( count == 2)
                        {
                            count = 0;
                            if(pub_sub_list_push(&contexts, field,  strlen(field)+1))
                                printf("Updated contexts list with %s and size %d\n", field, strlen(field)+1);
                        }
                    }
                  j = 0;
                }
                ++i; ++j;
            }
            break;
        }
    }*/
    pub_sub_list_init(&lids);
    pub_sub_list_init(&tags);
    pub_sub_list_init(&contexts);
    if( create_and_bind() != SUCCESS )
    {
        printf("Error while creating or binding sockets. Filter app exiting\n");
        exit(1);
    };
    Thread job_processor;
    pthread_create(&job_processor, NULL, &msg_jobs_handler, NULL);

	if( process_connections() == FAILURE );
    {
        printf("Error in process connections. Filter app exiting\n");
        exit(1);
    }
//    pthread_join(job_processor, NULL);
	return 0;
}