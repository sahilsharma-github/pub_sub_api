#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "pub_sub_util_types.h"
typedef bool (*string_comparator)(pub_sub_pair * p, char * key, char * value);
typedef bool (*cb_comparator)(pub_sub_cb_pair * p, char * key, pub_sub_callback val);

void pub_sub_list_init( pub_sub_list * l );
void pub_sub_cb_list_init( pub_sub_cb_list * l );
void pub_sub_cb_list_free( pub_sub_cb_list * l);
void pub_sub_list_free( pub_sub_list* l);
void pub_sub_cb_list_append(pub_sub_cb_list * l, pub_sub_cb_pair data);
void pub_sub_list_append(pub_sub_list * l, void * data, int elem_size);

Result pub_sub_list_push(pub_sub_list * l, void * data, int elem_size);

int get_pub_sub_cb_list_size(pub_sub_cb_list * l);
int get_pub_sub_list_size(pub_sub_list * l);
void get_pub_sub_cb_list( pub_sub_cb_list * l1, char * key, cb_comparator fptr, pub_sub_cb_list * l);
//void get_pub_sub_cb_list( pub_sub_cb_list l1, char * key, cb_comparator fptr, pub_sub_cb_list * l);
void get_pub_sub_string_list( pub_sub_list * l1, char * key, string_comparator fptr, pub_sub_list * l);
//int get_list_size(pub_sub_list *l);
/*
void list_insert(pair_node* head, pair data);
void list_delete(pair_node* head, pair data);
char* list_search(pair_node* head, pair data);
*/
#endif
