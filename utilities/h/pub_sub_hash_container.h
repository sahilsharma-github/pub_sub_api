#ifndef PUB_SUB_HASH_CONTAINER_H
#define PUB_SUB_HASH_CONTAINER_H

#include "pub_sub_util_defs.h"
#include "pub_sub_util_types.h"

void pub_sub_cb_hash_insert( pub_sub_cb_map * map, pub_sub_callback cb, int n_keys, char (*keyargs)[1024]);
void pub_sub_string_hash_insert( pub_sub_string_map * map, int val, int n_keys, char (*keyargs)[1024]);
//void hash_insert( pub_sub_string_map * map, int val, int n_keys, char (*keyargs)[1024]);      // usage: insert(map, 2, lid, tag) or insert(map, 1,context)
/* after concating the char *, pass it to hasher to compute the index and store the pair there
 *If a similar pair exists, dont store. No duplicate values.
 */
//int hash_search( hash_map map, int num_keys, ...);
/*
 * Return only the those pairs from __hashtable whose value matched the passed key
 */

void pub_sub_cb_hash_search( pub_sub_cb_map * map, int n_keys, char ** keyargs, pub_sub_cb_list * l);
//void pub_sub_cb_hash_search( pub_sub_cb_map * map, int n_keys, char (*keyargs)[1024], pub_sub_cb_list * l);
void pub_sub_string_hash_search( pub_sub_string_map * map, int n_keys, char ** keyargs, pub_sub_list * l);

void hash_delete( pub_sub_string_map map, int val, int num_keys, ...);
#endif
