#include <stdint.h>
#include "include/uthash.h"
#include "include/tpl.h"

#ifndef COMMON_H
#define COOMON_H

#define DEBUG_ON 1 

void debug(const char* format,...);


void perrorf(const char* format,...);


struct arg_command{
	char **argv;
	int32_t argc;
};

int arg_command_serialize(char **buf, int *len, const struct arg_command* cmd);

int arg_command_deserialize(struct arg_command *cmd, const char *buf, int len);




struct con_id_type{
    uint32_t src_ip;
    uint16_t src_port;
    uint32_t dst_ip;
    uint16_t dst_port;
};

struct con_info_type{
    struct con_id_type con_id;
    uint32_t isn; 
};

struct con_list_entry{
	struct con_id_type con_id;
    uint32_t isn; 
    UT_hash_handle hh;
};

int con_info_serialize(char **buf, int *len, const struct con_info_type *con_info);
int con_info_deserialize(struct con_info_type *con_info, const char *buf, int len);

 
#endif // COMMON_H