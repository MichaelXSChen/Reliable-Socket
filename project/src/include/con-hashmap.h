#ifndef CON_HASHMAP_H
#define CON_HASHMAP_H

#include "common.h"
#include "uthash.h"

struct con_list_entry{
	struct con_id_type con_id;
    uint32_t send_seq; 
    uint32_t recv_seq;
    uint16_t has_timestamp;
    uint32_t timestamp;

    uint32_t mss_clamp;
    uint32_t snd_wscale;
    uint32_t rcv_wscale;
    uint16_t has_rcv_wscale;

    uint16_t con_type;
    
    UT_hash_handle hh;
};

typedef struct con_list_entry con_list_entry;

int find_connection(con_list_entry **entry ,struct con_id_type *con);

int insert_connection(con_list_entry* entry);
int insert_connection_bytes(char* buf, int len);
int init_con_hashmap();

#endif //CON_HASHMAP_H