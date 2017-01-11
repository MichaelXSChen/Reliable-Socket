#include "include/common.h"
#include "include/uthash.h"
#include "include/con-hashmap.h"

#include <pthread.h>
#include <stdint.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>

con_list_entry *con_list;

int find_connection(con_list_entry **entry ,struct con_id_type *con){
	//debugf("Finding entry with src_ip:%" PRIu32", src_port:%"PRIu16", dst_ip:%"PRIu32", dst_port:%"PRIu16"", con->src_ip, con->src_port, con->dst_ip, con->dst_port);
	HASH_FIND(hh, con_list, con, sizeof(struct con_id_type), *entry);
	return 0;
}


int insert_connection(con_list_entry* entry){
	struct con_list_entry *tmp = NULL; 
	find_connection(&tmp, &(entry->con_id));
	debugf("trying to insert entry with src_ip=%"PRIu32", isn = %"PRIu32"", entry->con_id.src_ip, entry->isn);
	

	if (tmp == NULL){
		HASH_ADD(hh, con_list, con_id, sizeof(struct con_id_type), entry);
		debugf("\nInsert success!! Size of Hashtable: %d", HASH_COUNT(con_list));
	}
	else {
		//TODO: How to do now;

		debugf("conflict, isn = %"PRIu32"", tmp->isn);
	}
	return 0;
}

int insert_connection_bytes(char* buf, int len){
	int ret;
	struct con_info_type con_info;
	ret = con_info_deserialize(&con_info, buf, len);
	if (ret <0){
		perrorf("Failed to deserialize consensused req");
		return -1;
	}
	con_list_entry entry;
	entry.con_id = con_info.con_id;
	entry.isn = con_info.isn;
	ret = insert_connection(&entry);
	if (ret <0){
		perrorf("Failed to insert into hashmap");
		return ret;
	}
	return 0;
}

int init_con_hashmap(){
	con_list = NULL;
}