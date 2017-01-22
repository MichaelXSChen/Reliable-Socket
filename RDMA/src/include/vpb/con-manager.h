#ifndef CON_MANAGER_H
#define CON_MANAGER_H

#define SERVICE_PORT 4321
#include "common.h"
#include "../../../utils/uthash/uthash.h"

struct con_list_entry{
	struct con_id_type con_id;
    uint32_t send_seq;
    uint32_t recv_seq; 
    UT_hash_handle hh;
};

struct con_isn_entry{
	struct con_id_type con_id;
	uint32_t isn; 
	UT_hash_handle hh;
};

int get_isn(uint32_t *isn, struct con_id_type *con);

int save_isn(uint32_t isn, struct con_id_type *con);


int con_manager_init();


int handle_consensused_con(char* buf, int len);

int get_con_out_seq(uint32_t *seq, struct con_id_type *con);

#endif