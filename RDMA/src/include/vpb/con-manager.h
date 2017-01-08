#ifndef CON_MANAGER_H
#define CON_MANAGER_H

#define SERVICE_PORT 4321
#include "common.h"

struct con_list_entry{
	struct con_id_type con_id;
    uint32_t isn; 
    UT_hash_handle hh;
};

typedef struct con_list_entry con_list_entry;

int find_connection(con_list_entry **entry , struct con_id_type *con);


int insert_connection(con_list_entry* entry);

int con_manager_init();

#endif