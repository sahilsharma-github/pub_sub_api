#ifndef PUB_SUB_UTIL_TYPES_H
#define PUB_SUB_UTIL_TYPES_H

#include "pub_sub_util_defs.h"
#include "../../h/pub_sub_msg_defines.h"

typedef int (*hash)(char * key);

typedef struct
{
    char key[2*PUB_SUB_UTIL_MAX_STR_LEN];                // Can be a combination of two strings
    char value[PUB_SUB_UTIL_MAX_STR_LEN];                // to store FDs
}pub_sub_pair;

typedef struct
{
    char key[2*PUB_SUB_UTIL_MAX_STR_LEN];                // Can be a combination of two strings
    pub_sub_callback cb;                // to store FDs
}pub_sub_cb_pair;

struct cb__node;
struct cb_node
{
    pub_sub_cb_pair data;
    struct cb_node * next;
};
typedef struct cb_node pub_sub_cb_node;

struct l_node;
struct l_node
{
    void * data;
    struct l_node * next;
};
typedef struct l_node pub_sub_generic_node;

struct q_node;
struct q_node
{
    void * data;
    struct q_node * next;
    struct q_node * prev;
    //Byte message_type;
    //Byte message_sub_type;
    int fd;
};
typedef struct q_node pub_sub_queue_node;
typedef struct
{
    pub_sub_queue_node * rear;
    pub_sub_queue_node * front;
}pub_sub_generic_queue;

typedef struct
{
    int list_size;
    pub_sub_generic_node * head;
    pub_sub_generic_node * tail;
}pub_sub_list;

typedef struct
{
    int list_size;
    pub_sub_cb_node * head;
    pub_sub_cb_node * tail;
}pub_sub_cb_list;
/*
struct node;
struct node
{
    pair data;
    struct node * next;
};
typedef struct node pair_node;
*/
struct msg_node;
struct m_node
{
    generic_msg msg_data;
    struct m_node * next;
    struct m_node * prev;
};

typedef struct m_node msg_node;

typedef struct
{
    msg_node * rear;
    msg_node * front;
}pub_sub_queue;

struct buf_node;
struct b__node
{
    char *  msg_buf;
    struct b_node * next;
    struct b_node * prev;
};

typedef struct b_node buf_node;

typedef struct
{
    msg_node * rear;
    msg_node * front;
}pub_sub_msg_queue;


typedef struct
{
    pub_sub_cb_list hashtable[PUB_SUB_MAX_BUCKETS];
    hash hasher;				// add a default string hasher
}pub_sub_cb_map;

typedef struct
{
    pub_sub_list hashtable[PUB_SUB_MAX_BUCKETS];
    hash hasher;				// add a default string hasher
}pub_sub_string_map;
#endif