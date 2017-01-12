#ifndef CON_MANAGER_H
#define CON_MANAGER_H

#define SERVICE_PORT 4321
#include "common.h"

struct con_list_entry{
	struct con_id_type con_id;
    uint32_t send_seq;
    uint32_t recv_seq; 
    UT_hash_handle hh;
};

typedef struct con_list_entry con_list_entry;


int insert_connection_bytes(char* buf, int len);

int con_manager_init();

#endif