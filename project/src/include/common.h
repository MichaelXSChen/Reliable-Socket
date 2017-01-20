#include <stdint.h>
#include "tpl.h"

#ifndef COMMON_H
#define COMMON_H

#define DEBUG_ON 1 

#define CON_INFO_SERIAL_LEN 41

void debugf(const char* format,...);


void perrorf(const char* format,...);


struct command_t{
	char **argv;
	int32_t argc;
};

int command_t_serialize(char **buf, int *len, const struct command_t* cmd);

int command_t_deserialize(struct command_t *cmd, const char *buf, int len);




struct con_id_type{
    uint32_t src_ip;
    uint16_t src_port;
    uint32_t dst_ip;
    uint16_t dst_port;
};

struct con_info_type{
    struct con_id_type con_id;
    uint32_t send_seq; 
    uint32_t recv_seq;
    uint16_t has_timestamp;
    uint32_t timestamp;
};


int con_info_serialize(char **buf, int *len, const struct con_info_type *con_info);
int con_info_deserialize(struct con_info_type *con_info, const char *buf, int len);

struct con_info_reply{
    uint8_t is_leader;
    uint32_t isn;
};

int recv_bytes(int sk, char** buf, int *length);
int send_bytes(int sk, char* buf, int len);

int show_socket_info(int sk);

#endif // COMMON_H