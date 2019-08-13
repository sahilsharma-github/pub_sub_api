#include "../h/pub_sub_api.h"
#include<net/if.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/ioctl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include "../utilities/h/pub_sub_queue.h"

pub_sub_cb_map lidtag_map;
pub_sub_cb_map tag_map;
pub_sub_cb_map context_map;

pthread_t conn_manager;
pthread_mutex_t pub_sub_mutex = PTHREAD_MUTEX_INITIALIZER;;
bool thread_spawned = false;
pub_sub_queue msg_jobs;

static int sockfd;
static int regfd;
static int client_fd = FAILURE;
static struct sockaddr_in sendaddr;
static struct sockaddr_in regaddr;
static struct sockaddr_in sub_sendaddr;

static char interface_name[PUB_SUB_MAX_IF_LEN];             
static char interface_bcast_ip[PUB_SUB_MAX_IP_LEN];
//static char list[2*PUB_SUB_MAX_IP_LEN];

//void init();
char* extract_ip(char * if_name);
void convert_to_bcast_ip(char * ip);
void send_broadcast_message(char * msg, int size, const char * bcast_ip, int fd);
char * process_message_on_register_fd(char * buf);
void * tcp_conn_handler(void * data);
Result init_publish_api(const char * if_name)
{

    static bool initialized = false;
    if( !initialized )
    {
        if ((regfd = socket(PF_INET, SOCK_DGRAM, 0)) == FAILURE)
        {
            perror("socket creation fsilure");
            return FAILURE;
        }
        if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == FAILURE)
        {
            perror("socket");
            return FAILURE;
        }
        printf("Sockets created\n");

        memset(&sendaddr, 0, sizeof sendaddr);
        sendaddr.sin_family = AF_INET;
        sendaddr.sin_port = htons(PUB_BROADCAST_PORT);
        sendaddr.sin_addr.s_addr = INADDR_BROADCAST;

        memset(&regaddr, 0, sizeof regaddr);
        regaddr.sin_family = AF_INET;
        regaddr.sin_port = htons(REG_BROADCAST_PORT);
        regaddr.sin_addr.s_addr = INADDR_BROADCAST;

	    int optval = 1, ret_val;
        ret_val = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
        if( ret_val < 0)
        {
            perror("setsockopt error:\n");
            return FAILURE;
        }
        ret_val = setsockopt(regfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval));
        if( ret_val < 0)
        {
            perror("setsockopt error:\n");
            return FAILURE;
        }

        initialized = true;
    }
    if( strlen(if_name) >= PUB_SUB_MAX_IF_LEN )
    {
        printf("Very large string passed for interface name! Returning failure");
        return FAILURE;
           
    }
    char interface_ip[PUB_SUB_MAX_IP_LEN];
    strcpy(interface_name, if_name);
    strcpy(interface_ip, extract_ip(interface_name));
    convert_to_bcast_ip(interface_ip);
    return SUCCESS;

}

// // To extract IP string configured for NW interface if_name
char* extract_ip(char * if_name)
{
    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM,0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ-1);
    ioctl( fd, SIOCGIFADDR, &ifr);
    close(fd);
    return inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr);

}
void convert_to_bcast_ip(char * ip)
{
    int count = 0, i = 0;
    char * bcast_ip = (char*)malloc(strlen(ip)+2);
    strcpy(bcast_ip, ip);
    printf("bcast_ip: %s\n", bcast_ip);

    while( count != 3 && i < strlen(bcast_ip))
    {
        if( bcast_ip[i] == '.')
            ++count;
        ++i;
    }
    bcast_ip[i++] = '2';
    bcast_ip[i++] = '5';
    bcast_ip[i++] = '5';
    bcast_ip[i] = '\0';
    strcpy(interface_bcast_ip, bcast_ip);
    printf("bcast_ip: %s\n", interface_bcast_ip);
    free(bcast_ip);
}

void send_broadcast_message(char * msg, int size, const char * bcast_ip, int fd)
{
    static int count = 0;
    sendaddr.sin_addr.s_addr = inet_addr(bcast_ip);
    int numbytes;
    if( fd == regfd )
        numbytes = sendto(fd, msg, size, 0, (struct sockaddr *)&regaddr, sizeof regaddr);
    else if( fd == sockfd)
        numbytes = sendto(fd, msg, size, 0, (struct sockaddr *)&sendaddr, sizeof sendaddr);
    if( numbytes < 0 )
        printf("Error in broadcasting the packet.\n");
	printf("Broadcasted a packet of size: %d on Broadcast IP %s\n", size, bcast_ip);
    /*if( numbytes>0)
        ++count;
	printf("Broadcasted a packet of size: %d on Broadcast IP %s\n", size, bcast_ip);
    if( count == 100000)
        printf("count is %d\n", count);*/
}

void publish_info(const char * logical_id, const char * value_tag, const char * value)
{   
    generic_msg * msg = malloc(sizeof(generic_msg));
    msg->hdr.message_type = DATA;
	msg->hdr.message_sub_type = PUBLISH_DATA;
	msg->hdr.n_tlv = 3;
	s_tlv lid, tag,val;
	lid.ie_type = STRING;
	lid.ie_length = strlen(logical_id)+1;
	strcpy(lid.ie_value, logical_id);

	tag.ie_type = STRING;
	tag.ie_length = strlen(value_tag)+1;
	strcpy(tag.ie_value, value_tag);

	val.ie_type = STRING;
	val.ie_length = strlen(value)+1;
	strcpy(val.ie_value, value);

	msg->fields[0] = lid;
    msg->fields[1] = tag;
    msg->fields[2] = val;
	int msg_size = -1;
	int * ptr = &msg_size;	
	char * buf = encode(ptr, msg);
    send_broadcast_message(buf, *ptr, interface_bcast_ip, sockfd);
	free(buf);
    free(msg);

}
void publish_cinfo(const char * logical_id, const char * value_tag, char * context, const char * value)
{
    //generic_msg msg;
    generic_msg * msg = malloc(sizeof(generic_msg));
    msg->hdr.message_type = DATA;
    msg->hdr.message_sub_type = PUBLISH_DATA;
    msg->hdr.n_tlv = 4;
    s_tlv lid, tag, ctxt, val;
    lid.ie_type = STRING;
    lid.ie_length = strlen(logical_id)+1;
    strcpy(lid.ie_value, logical_id);

    tag.ie_type = STRING;
    tag.ie_length = strlen(value_tag)+1;
    strcpy(tag.ie_value, value_tag);

    ctxt.ie_type = CONTEXT_STRING;
    ctxt.ie_length = strlen(context)+1;
    strcpy(ctxt.ie_value, context);

    val.ie_type = STRING;
    val.ie_length = strlen(value)+1;
    strcpy(val.ie_value, value);

    msg->fields[0] = lid;
    msg->fields[1] = tag;
    msg->fields[2] = ctxt;
    msg->fields[3] = val;
    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    send_broadcast_message(buf, *ptr, interface_bcast_ip, sockfd);
    free(buf);
    free(msg);


}

// RUn valgring on python to verify, this doesnt release memory
// Calling function will have to release memory
char * get_all_logical_ids()
{
    char recvbuf[PUB_SUB_MAX_STR_LEN*2];
    generic_msg * msg = malloc(sizeof(generic_msg));
    msg->hdr.message_type = CONTROL;
    msg->hdr.message_sub_type = RETRIEVE_LOGICAL_ID_LIST;
    msg->hdr.n_tlv = 0;

    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    send_broadcast_message(buf, *ptr, interface_bcast_ip, regfd);
    free(buf);
    free(msg);
    char str[PUB_SUB_MAX_STR_LEN*50];                
    char * ret_str;
    int index = 0;
    while(true)
    {
        int n_bytes = recvfrom(regfd, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
        printf("Register get_alllids reply received of size %d\n", n_bytes);
        ret_str = process_message_on_register_fd(recvbuf);
        if( index+strlen(ret_str)-1 < PUB_SUB_MAX_STR_LEN*50)
            strcpy(str+index, ret_str);
        else
        {
            printf("Too many LIDs\n");
            return NULL;
        }
        /*for( int j = 1010 ; j < 1021 ; ++j)
            printf("char at %d is %d and %c\n", j, ret_str[j], ret_str[j]);
        for( int j = 0 ; j < 21 ; ++j)
            printf("char at %d is %d and %c\n", j, ret_str[j], ret_str[j]);
        printf("strlen is %d\n", strlen(ret_str));
        printf("checking at %d \n", strlen(ret_str)-2);*/
        if( ret_str[strlen(ret_str)-2] == 6)
        {
            //printf("ACK is there\n");
            index += strlen(ret_str);
            free(ret_str);
            continue;
        }
        else
            break;
        
    }
    free(ret_str);
    /*printf("strlen str:  %d\n", strlen(str));
    for(int i = 1510; i <=1519;++i)
        printf("Char at %d is %d and %c\n", i, str[i], str[i]);
    printf("From get_all_logical_ids:  %s\n", str);
    printf("Returning to Python");*/
    char * res = malloc(strlen(str)+1);
    strcpy(res, str);
    return res;
}

char * get_all_tags()
{
    char recvbuf[PUB_SUB_MAX_STR_LEN*2];
    generic_msg * msg = malloc(sizeof(generic_msg));
    msg->hdr.message_type = CONTROL;
    msg->hdr.message_sub_type = RETRIEVE_TAG_LIST;
    msg->hdr.n_tlv = 0;

    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    send_broadcast_message(buf, *ptr, interface_bcast_ip, regfd);
    free(buf);
    free(msg);

    char str[PUB_SUB_MAX_STR_LEN*50];
    char * ret_str;
    int index = 0;
    while(true)
    {
        int n_bytes = recvfrom(regfd, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
        printf("Register get_alllids reply received of size %d\n", n_bytes);
        ret_str = process_message_on_register_fd(recvbuf);
        if( index+strlen(ret_str)-1 < PUB_SUB_MAX_STR_LEN*50)
            strcpy(str+index, ret_str);
        else
        {
            printf("Too many Tags\n");
            return NULL;
        }
        /*for( int j = 1010 ; j < 1021 ; ++j)
            printf("char at %d is %d and %c\n", j, ret_str[j], ret_str[j]);
        for( int j = 0 ; j < 21 ; ++j)
            printf("char at %d is %d and %c\n", j, ret_str[j], ret_str[j]);
        printf("strlen is %d\n", strlen(ret_str));
        printf("checking at %d \n", strlen(ret_str)-2);*/
        if( ret_str[strlen(ret_str)-2] == 6)
        {
            //printf("ACK is there\n");
            index += strlen(ret_str);
            free(ret_str);
            continue;
        }
        else
            break;

    }
    free(ret_str);
    /*printf("strlen str:  %d\n", strlen(str));
    for(int i = 1510; i <=1519;++i)
        printf("Char at %d is %d and %c\n", i, str[i], str[i]);
    printf("From get_all_tags:  %s\n", str);
    printf("Returning to Python");*/
    char * res = malloc(strlen(str)+1);
    strcpy(res, str);
    return res;

}


char * get_all_contexts()
{
    char recvbuf[PUB_SUB_MAX_STR_LEN*2];
    generic_msg * msg = malloc(sizeof(generic_msg));
    msg->hdr.message_type = CONTROL;
    msg->hdr.message_sub_type = RETRIEVE_CONTEXT_LIST;
    msg->hdr.n_tlv = 0;

    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    send_broadcast_message(buf, *ptr, interface_bcast_ip, regfd);
    free(buf);
    free(msg);
    
    char str[PUB_SUB_MAX_STR_LEN*50];
    char * ret_str;
    int index = 0;
    while(true)
    {
        int n_bytes = recvfrom(regfd, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
        printf("Register get_alllids reply received of size %d\n", n_bytes);
        ret_str = process_message_on_register_fd(recvbuf);
        if( index+strlen(ret_str)-1 < PUB_SUB_MAX_STR_LEN*50)
            strcpy(str+index, ret_str);
        else
        {
            printf("Too many Contexts\n");
            return NULL;
        }
    /*    for( int j = 1010 ; j < 1021 ; ++j)
            printf("char at %d is %d and %c\n", j, ret_str[j], ret_str[j]);
        for( int j = 0 ; j < 21 ; ++j)
            printf("char at %d is %d and %c\n", j, ret_str[j], ret_str[j]);
        printf("strlen is %d\n", strlen(ret_str));
        printf("checking at %d \n", strlen(ret_str)-2);*/
        if( ret_str[strlen(ret_str)-2] == 6)
        {
            //printf("ACK is there\n");
            index += strlen(ret_str);
            free(ret_str);
            continue;
        }
        else
            break;

    }
    free(ret_str);
    /*printf("strlen str:  %d\n", strlen(str));
    for(int i = 1510; i <=1519;++i)
        printf("Char at %d is %d and %c\n", i, str[i], str[i]);
    printf("From get_all_contexts:  %s\n", str);
    printf("Returning to Python");*/
    char * res = malloc(strlen(str)+1);
    strcpy(res, str);
    return res;

}



void register_lid(const char * logical_id)
{
    generic_msg * msg = malloc(sizeof(generic_msg));
    msg->hdr.message_type = CONTROL;
    msg->hdr.message_sub_type = REGISTER_DATA;
    msg->hdr.n_tlv = 1;
    s_tlv lid;
    lid.ie_type = STRING;
    lid.ie_length = strlen(logical_id)+1;
    strcpy(lid.ie_value, logical_id);

    msg->fields[0] = lid;
    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    if( !thread_spawned )
    {
        pthread_create(&conn_manager, NULL, &tcp_conn_handler, NULL);
        thread_spawned = true;
        printf("Thread spawned\n");
         //pthread_join(conn_manager, NULL);            // how and when to join for this thread
    }
    else
        printf("Thread already spawned\n");

    send_broadcast_message(buf, *ptr, interface_bcast_ip, regfd);
    free(buf);
    free(msg);

}

void register_lid_tag(const char * logical_id, const char * value_tag)
{
    generic_msg * msg = malloc(sizeof(generic_msg));
    msg->hdr.message_type = CONTROL;
    msg->hdr.message_sub_type = REGISTER_DATA;
    msg->hdr.n_tlv = 2;
    s_tlv lid, tag;
    lid.ie_type = STRING;
    lid.ie_length = strlen(logical_id)+1;
    strcpy(lid.ie_value, logical_id);

    tag.ie_type = STRING;
    tag.ie_length = strlen(value_tag)+1;
    strcpy(tag.ie_value, value_tag);

    msg->fields[0] = lid;
    msg->fields[1] = tag;
    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    if( !thread_spawned )
    {
        pthread_create(&conn_manager, NULL, &tcp_conn_handler, NULL);
        thread_spawned = true;
        printf("Thread spawned\n");
         //pthread_join(conn_manager, NULL);            // how and when to join for this thread
    }
    else
        printf("Thread already spawned\n");

    send_broadcast_message(buf, *ptr, interface_bcast_ip, regfd);
    free(buf);
    free(msg);

}

void register_lid_tag_context(const char * logical_id, char * value_tag, const char * context)
{
    generic_msg * msg = malloc(sizeof(generic_msg));
    msg->hdr.message_type = CONTROL;
    msg->hdr.message_sub_type = REGISTER_DATA;
    msg->hdr.n_tlv = 3;

    s_tlv lid, tag, ctxt;
    lid.ie_type = STRING;
    lid.ie_length = strlen(logical_id)+1;
    strcpy(lid.ie_value, logical_id);

    tag.ie_type = STRING;
    tag.ie_length = strlen(value_tag)+1;
    strcpy(tag.ie_value, value_tag);

    ctxt.ie_type = CONTEXT_STRING;
    ctxt.ie_length = strlen(context)+1;
    strcpy(ctxt.ie_value, context);

    msg->fields[0] = lid;
    msg->fields[1] = tag;
    msg->fields[2] = ctxt;
    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    if( !thread_spawned )
    {
        pthread_create(&conn_manager, NULL, &tcp_conn_handler, NULL);
        thread_spawned = true;
        printf("Thread spawned\n");
         //pthread_join(conn_manager, NULL);            // how and when to join for this thread
    }
    else
        printf("Thread already spawned\n");

    send_broadcast_message(buf, *ptr, interface_bcast_ip, regfd);
    free(buf);
    free(msg);

}

Result init_client()
{
    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( client_fd < 0 )
    {
        perror("socket creation failure:\n");
        return FAILURE;
    }
    printf("Client Socket created\n");
    memset(&sub_sendaddr, 0, sizeof sub_sendaddr);
    sub_sendaddr.sin_family = AF_INET;
    sub_sendaddr.sin_port = htons(SUB_FILTER_PORT);
    //char * serv_ip = "192.168.121.2";
    //char * serv_ip = "192.168.2.3";
    //sub_sendaddr.sin_addr.s_addr = inet_addr(serv_ip);
    sub_sendaddr.sin_addr.s_addr = htonl(INADDR_ANY);       // cjhange it to filter app ip

    int optval = 1, ret_val;
    ret_val = setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if( ret_val < 0)
    {
        perror("setsockopt error:\n");
        return FAILURE;
    }
    return SUCCESS;
}

void process_notification_msg(generic_msg * msg)
{
    char * keys[2];
    pub_sub_cb_list cb_list1;
    pub_sub_cb_list cb_list2;
    pub_sub_cb_list cb_list3;
    pub_sub_cb_list_init(&cb_list1);
    pub_sub_cb_list_init(&cb_list2);
    pub_sub_cb_list_init(&cb_list3);
    char * lid; 
    char * tag;
    char * value;
    lid = msg->fields[0].ie_value;                // for swig
    tag = msg->fields[1].ie_value;                // for swig
    if( msg->hdr.n_tlv < 3)
        printf("Notify Data message receievd with insufficient data. DIscarding!\n");
    else if( msg->hdr.n_tlv == 3)
    {
        value = msg->fields[2].ie_value;              // for swig
        keys[0] = msg->fields[0].ie_value;
        keys[1] = msg->fields[1].ie_value;
        pub_sub_cb_hash_search( &lidtag_map, 2, keys, &cb_list1);
        keys[0] = msg->fields[1].ie_value;
        pub_sub_cb_hash_search( &tag_map, 1, keys, &cb_list2);
    }
    else if( msg->hdr.n_tlv == 4)
    {
        value = msg->fields[3].ie_value;                  // for swig
        keys[0] = msg->fields[0].ie_value;
        keys[1] = msg->fields[1].ie_value;
        pub_sub_cb_hash_search( &lidtag_map, 2, keys, &cb_list1);
        keys[0] = msg->fields[1].ie_value;
        pub_sub_cb_hash_search( &tag_map, 1, keys, &cb_list2);
        keys[0] = msg->fields[2].ie_value;
        pub_sub_cb_hash_search( &context_map, 1, keys, &cb_list3);
    }
    if( get_pub_sub_cb_list_size( &cb_list1) == NO_ITEMS && get_pub_sub_cb_list_size( &cb_list2) == NO_ITEMS && 
                            get_pub_sub_cb_list_size( &cb_list3) == NO_ITEMS )
    {
        printf("All maps are empty. No one subscribed for this data.\n");
        return;
    }
    //printf("size1: %d  size2: %d  size3: %d ", get_pub_sub_cb_list_size( &cb_list1), get_pub_sub_cb_list_size( &cb_list2),  get_pub_sub_cb_list_size( &cb_list3));
    int n_callbacks = 0;
    pub_sub_callback arr_cb[PUB_SUB_MAX_BUCKETS];
    pub_sub_cb_node * node = cb_list1.head;
    while( node != NULL )
    {
        arr_cb[n_callbacks++] = node->data.cb;
        node = node->next;
    }
    node = cb_list2.head;
    while( node != NULL )
    {
        arr_cb[n_callbacks++] = node->data.cb;
        node = node->next;
    }
    node = cb_list3.head;
    while( node != NULL )
    {
        arr_cb[n_callbacks++] = node->data.cb;
        node = node->next;
    }
    for( int i = 0 ; i < n_callbacks-1 ; ++i )
    {   
        for(int j = i+1 ; j < n_callbacks ; ++j)
        {
            if( arr_cb[i] == arr_cb[j] )
                arr_cb[j] = NULL;
        }
    }
    //printf("number of callbacks to be invoked: %d\n", n_callbacks);
    for(int i = 0 ; i < n_callbacks ; ++i)
    {
        if( arr_cb[i] != NULL )
            arr_cb[i](lid, tag, value);
    }
    pub_sub_cb_list_free(&cb_list1);
    pub_sub_cb_list_free(&cb_list2);
    pub_sub_cb_list_free(&cb_list3);
}

void process_msg_from_filter_app(generic_msg * msg)
{
    if( msg == NULL)
        return;
    switch( msg->hdr.message_type )
    {
        //case DATA:
        case 2:
            switch(msg->hdr.message_sub_type)
            {
                case 2:
                //case NOTIFY_DATA:
                    //printf("Data Notification message received from filter!\n");
                    process_notification_msg(msg);
                    break;
            }
            break;
        default:
            printf("Message with unknown message type received. Ignore!\n");
            break;
    }
}


void update_map( pub_sub_cb_map * map, generic_msg msg, pub_sub_callback cb)
{
    char keys[2][PUB_SUB_MAX_STR_LEN];
    if ( msg.hdr.n_tlv == 2 )
    {
        strcpy( keys[0], msg.fields[0].ie_value);
        strcpy( keys[1], msg.fields[1].ie_value);
        pub_sub_cb_hash_insert(map, cb, 2, keys);
    }
    else if( msg.hdr.n_tlv == 1 )
    {
        strcpy( keys[0], msg.fields[0].ie_value);
        pub_sub_cb_hash_insert(map, cb, 1, keys);
    }
    else
    {
        printf("Unexpected number of fields received while update callback map. Returning!");
        return;
    }
}

char * process_retrieve_msg(generic_msg * msg)
{
    int index = 0;
    /*printf("ie_length is %d\n", msg->fields[0].ie_length);
    for(int i = 0 ; i < msg->fields[0].ie_length ; ++i)
        printf("%c", msg->fields[0].ie_value[i]);
    printf("\n");
    for(int i = 0 ; i < msg->fields[0].ie_length ; ++i)
        printf("%d", msg->fields[0].ie_value[i]);
    printf("\n");*/
    char * list = malloc(msg->hdr.msg_size);                    // memory leak. Needs to be freed
    //char list[2048];
    //int i = 0;
    list[0] = '\0';
    //printf("List is : \n[");
    //printf("msg->fields[0].ie_length is: %d\n", msg->fields[0].ie_length);
    while( index < msg->fields[0].ie_length)
    {
        //printf("%s , ", msg->fields[0].ie_value+index);
        strcat(list, msg->fields[0].ie_value+index);
        strcat(list, ";");
        index += strlen(msg->fields[0].ie_value+index);
        index += 2;
    }
    //printf(" ]\n");
    //printf("str is : %s\n", list);
    return list;
}

char * process_message_on_register_fd(char * buf)
{
    generic_msg * msg = malloc(sizeof(generic_msg));
    decode(buf, msg, 0);
    char * str = NULL;
    if( msg->hdr.message_type == DATA )
    {
        if( msg->hdr.message_sub_type == RETRIEVE_LOGICAL_ID_LIST || msg->hdr.message_sub_type == RETRIEVE_TAG_LIST 
            || msg->hdr.message_sub_type == RETRIEVE_CONTEXT_LIST)
        {
            str = process_retrieve_msg(msg);
        }
        else
            printf("Unknown DATA msg received\n");

    }
    else if( msg->hdr.message_type == CONTROL )
        printf("CONTROL msg received. Potentially wrong type. Discarding.\n");
    else
        printf("Unknow msg received\n");
    free(msg);
    return str;

}
void * tcp_conn_handler(void * data)            
{
    printf("Thread invoked\n");
    if( init_client() == FAILURE )
    {
        printf("Error in init_client. Thread exiting\n");
        pthread_exit(NULL);
    } 
    if (connect(client_fd, (struct sockaddr *)&sub_sendaddr, sizeof(sub_sendaddr)) < 0)
    {   
        perror("Failed to establish connection with filter app. Thread Exiting\n");
        pthread_exit(NULL);
    }
    printf("Established connection with filter app\n");
    while(true)
    {
        pthread_mutex_lock(&pub_sub_mutex);
        generic_msg * msg = pop(&msg_jobs);
        pthread_mutex_unlock(&pub_sub_mutex);
        if( msg != NULL )
        {
            //send_subscribe_req(msg);
            int msg_size = -1;
            int * ptr = &msg_size;
            char * buf = encode(ptr, msg);
            if( send(client_fd, buf, *ptr, 0) < 0)
            {
                printf("Error while sending subscribe msg to filter app\n");
            }
            printf("Subscribe request message of size %d sent to filter app\n", *ptr);
            /*for( int i = 0 ; i < *ptr; ++i) 
                printf("char at %d is %c\n", i, buf[i]);*/
            free(buf);
        }
        else
            break;  
        free(msg);  
    }
    fd_set read_set;
    generic_msg * msg = malloc(sizeof(generic_msg));
    char recvbuf[PUB_SUB_MAX_STR_LEN];
    int ret_val;
    struct timeval timeout;
    //timeout.tv_sec = 5;
    //timeout.tv_usec = 1;
    //bool flag = false;
    for(;;)
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = 5;
        FD_ZERO(&read_set);
        FD_SET(client_fd, &read_set);
        //FD_SET(regfd, &read_set);
        //printf("\n\n---------------------------------------------------------------------------------------------\n");
        ret_val = select(client_fd+1, &read_set, NULL, NULL, &timeout);      
        //ret_val = select(client_fd+1, &read_set, NULL, NULL, NULL);      
        //printf("Select returned\n");
        if( ret_val < 0 )
        {
            perror("Client - Select failed\n");
            close(client_fd);
            printf("subscriber thread exiting\n");
            pthread_exit(NULL);
        }
        /*else if( FD_ISSET(regfd, &read_set) )   
        {
            int n_bytes = recvfrom(regfd, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
            printf("Register reply received of size %d\n", n_bytes);
            process_message_on_register_fd(recvbuf);
        }*/
        else if( ret_val == 1 && FD_ISSET(client_fd, &read_set))
        {
            //printf("\n\n---------------------------------------------------------------------------------------------\n");
            //printf("Data received from server...\n");
            int index = 0, n_bytes = 0, recv_len = 0, bytes_in_buffer = 0;
            uint_16 msg_size = 0;
            n_bytes = recv(client_fd, recvbuf, sizeof(recvbuf), 0);
            bytes_in_buffer = n_bytes;
                    //printf("Since , exiting\n");
                    //exit(1);
            while( true )
            {
                printf("n_bytes from recv call is %d\n", n_bytes);
                if( n_bytes < 0)
                {
                    perror("Error while receving from filter app: Killing thread\n");
                    close(client_fd);
                    //break;
                    pthread_exit(NULL);
                }
                if( n_bytes == 0 )
                {
                    printf("Connection closed. Killing thread\n");
                    close(client_fd); 
                    pthread_exit(NULL);
                    //break;
                }
                //n_bytes += recv_len;
                if( n_bytes != 1 )
                    msg_size = get_msg_size(recvbuf, index);
                else 
                {
                    static int count = 0;
                    printf("printf count for this is %d\n", ++count);    
                    printf("index: %d, recvbuf[0]: %c and %d  and and recvbuf[index]: %c and %d\n", index, recvbuf[0], recvbuf[0], recvbuf[index], recvbuf[index]);
                    printf("doing recvbuf[0] = recvbuf[index]\n");
                    printf("index at here is : %d and recvbuf[index] is %c, %d \n", index, recvbuf[index], recvbuf[index]);
                    recvbuf[0] = recvbuf[index];
                    printf("n_bytes before: %d\n", n_bytes);
                    printf("size passed to recv: %d\n", sizeof(recvbuf)-1);
                    n_bytes += recv(client_fd, recvbuf+1, (sizeof(recvbuf)-1), 0);
                    bytes_in_buffer = n_bytes;
                    if(n_bytes > 1)
                    {
                        index = 0;
                        recv_len = 0;
                        msg_size = get_msg_size(recvbuf, index);
                    }
                    printf("msg_size: %d\n", msg_size);
                    for(int k = 0 ;k < msg_size ;++k)
                        printf("char at %d is %d or %c\n", k, recvbuf[k], recvbuf[k]);
                }
                if( msg_size == n_bytes)
                {
                    //printf("Since msg_size == n_bytes, exiting\n");
                    //pthread_exit(NULL);
                    printf("decoding the msg\n");
                    //generic_msg  msg;
                     decode(recvbuf, msg, index);
                    printf("decoded the msg\n");
                    process_msg_from_filter_app(msg);
                    break;
                }
                else if( msg_size > n_bytes)
                {
                    printf("bytes_in_buffer: %d\n", bytes_in_buffer);
                    for(int j = index, k = 0 ; j < bytes_in_buffer; ++j, ++k)
                    {
                        printf("recvbuf at %d is %d or %c\n", k, recvbuf[k], recvbuf[k]);
                        printf("recvbuf at %d is %d or %c\n", j, recvbuf[j], recvbuf[j]);
                        recvbuf[k] = recvbuf[j];
                    }
                    index = 0;
                    recv_len = n_bytes;
                    n_bytes = recv(client_fd, recvbuf+recv_len, sizeof(recvbuf)-recv_len, 0);
                    bytes_in_buffer = n_bytes+recv_len;
                    printf("n_bytes from next recv call >> is %d\n", n_bytes);
                    printf("After next recv\n\n");
                    for(int m = 0 ;m < 11 ;++m)
                        printf("char at %d is %d or %c\n", m, recvbuf[m], recvbuf[m]);
                    printf("recvlen is %d\n", recv_len);
                    for(int m = recv_len ;m < recv_len+11 ;++m)
                        printf("char at %d is %d or %c\n", m, recvbuf[m], recvbuf[m]);

                    if( n_bytes == 0 )
                    {
                        printf("Connection closed. Killing subscriber thread.\n");
                        close(client_fd); 
                        //break;
                        pthread_exit(NULL);
                    }
                    n_bytes += recv_len;
                    continue;
                }
                else
                {
                    /*for(int m = 0 ; m < msg_size+3 ;++m)
                        printf("recvbuf at index %d is %c, %d\n", m, recvbuf[m], recvbuf[m]);
                    generic_msg msg;*/
                    printf("msg size for is: %d", msg_size);
                     decode(recvbuf, msg, index);
                    process_msg_from_filter_app(msg);
                    while( true )
                    {    
                        index += msg_size;
                        n_bytes -= msg_size;
                        if( n_bytes != 1)
                            msg_size = get_msg_size(recvbuf, index);
                        else
                            break;
                        printf("Here 2 msg_size: %d\n", msg_size);
                        //printf("msg_size is: %d\n", msg_size);
                        printf("index is: %d\n", index);
                        printf("n_bytes is: %d\n", n_bytes);
                        if( msg_size <= n_bytes)
                        {
                            //generic_msg msg;
                             decode(recvbuf, msg, index);
                            printf("decoding next msg\n");
                            process_msg_from_filter_app(msg);
                            if( msg_size ==  n_bytes)
                                break; 
                            msg_size = get_msg_size(recvbuf, index);
                        }
                        else 
                            break;
                    }
                    if( msg_size ==  n_bytes)
                        break;
                }
            }
        }
        else if( ret_val == 0)
        {
            //printf("Select timed out...\n");
            pthread_mutex_lock(&pub_sub_mutex);
            generic_msg * msg1 = pop(&msg_jobs);
            pthread_mutex_unlock(&pub_sub_mutex);
            if( msg1 == NULL )
                continue;
            else
            {
                int msg_size = -1;
                int * ptr = &msg_size;
                char * buf = encode(ptr, msg1);
                if( send(client_fd, buf, *ptr, 0) < 0)
                {
                    printf("Error while sending subscribe msg to filter app\n");
                }
                else
                    printf("Subscribe request message of size %d sent to filter app\n", *ptr);
                /*for( int i = 0 ; i < *ptr; ++i)
                    printf("char at %d is %c\n", i, buf[i]);*/
                free(buf);
                free(msg1);

            } 
        }
    }
    return NULL;
}
/*
void send_subscribe_req(generic_msg * msg)
{
    int msg_size = -1;
    int * ptr = &msg_size;
    char * buf = encode(ptr, msg);
    if( send(client_fd, buf, *ptr, 0) < 0)
    {
        printf("Error while sending subscribe msg to filter app\n");
        return -1;
    }
    else
        printf("Subscribe request message of size %d sent to filter app\n", *ptr);

    free(buf);
}
*/
void subscribe_tag( pub_sub_callback cb, char * value_tag)
{
    printf("In subscribeForTag method ...\n");
    generic_msg msg;
    msg.hdr.message_type = CONTROL;
    msg.hdr.message_sub_type = SUBSCRIBE_REQUEST;
    msg.hdr.n_tlv = 1;
    s_tlv tag;
    tag.ie_type = STRING;
    tag.ie_length = strlen(value_tag)+1;
    strcpy(tag.ie_value, value_tag);
    msg.fields[0] = tag;

    if( !thread_spawned )
    {
        push(msg, &msg_jobs);
        update_map(&tag_map, msg, cb);
        pthread_create(&conn_manager, NULL, &tcp_conn_handler, NULL);
        thread_spawned = true;
        printf("Thread spawned\n");
    }
    else
    {
        printf("Thread already spawned\n");
        pthread_mutex_lock(&pub_sub_mutex);
        push(msg, &msg_jobs);
        update_map(&tag_map, msg, cb);
        //send_subscribe_req(&msg);
        pthread_mutex_unlock(&pub_sub_mutex);
    }

}
void subscribe_context( pub_sub_callback cb, char * ctxt)
{
    printf("In subscribeForContext method ...\n");
    generic_msg msg;
    msg.hdr.message_type = CONTROL;
    msg.hdr.message_sub_type = SUBSCRIBE_REQUEST;
    msg.hdr.n_tlv = 1;
    s_tlv context;
    context.ie_type = CONTEXT_STRING;
    context.ie_length = strlen(ctxt)+1;
    strcpy(context.ie_value, ctxt);
    msg.fields[0] = context;

    if( !thread_spawned )
    {
        push(msg, &msg_jobs);
        update_map(&context_map, msg, cb);
        pthread_create(&conn_manager, NULL, &tcp_conn_handler, NULL);
        thread_spawned = true;
        printf("Thread spawned\n");
    }
    else
    {
        printf("Thread already spawned\n");
        pthread_mutex_lock(&pub_sub_mutex);
        push(msg, &msg_jobs);
        update_map(&context_map, msg, cb);
        //send_subscribe_req(&msg);
        pthread_mutex_unlock(&pub_sub_mutex);
    }

}
void subscribe_lid_tag( pub_sub_callback cb, char * logical_id, char * value_tag)
{
    printf("In subscribeForLidTag method with lid %s and tag %s ...\n", logical_id, value_tag);
    generic_msg msg;
    msg.hdr.message_type = CONTROL;
    msg.hdr.message_sub_type = SUBSCRIBE_REQUEST;
    msg.hdr.n_tlv = 2;
    s_tlv lid, tag;
    lid.ie_type = STRING;
    lid.ie_length = strlen(logical_id)+1;
    strcpy(lid.ie_value, logical_id);

    tag.ie_type = STRING;
    tag.ie_length = strlen(value_tag)+1;
    strcpy(tag.ie_value, value_tag);
    msg.fields[0] = lid;
    msg.fields[1] = tag;
    
    if( !thread_spawned )
    {
        push(msg, &msg_jobs);
        update_map(&lidtag_map, msg, cb);
        pthread_create(&conn_manager, NULL, &tcp_conn_handler, NULL);    
        thread_spawned = true;
        printf("Thread spawned\n");     
         //pthread_join(conn_manager, NULL);            // how and when to join for this thread
    }
    else
    {
        printf("Thread already spawned\n");     
        pthread_mutex_lock(&pub_sub_mutex);

        push(msg, &msg_jobs);
        update_map(&lidtag_map, msg, cb);
        //send_subscribe_req(&msg);
        pthread_mutex_unlock(&pub_sub_mutex);    
           // who will join this thread??
    }
}