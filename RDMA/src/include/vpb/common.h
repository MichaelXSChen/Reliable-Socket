#include <stdint.h>
#include "tpl.h"



#define DEBUG_ON 1 

#ifndef COMMON_H
#define COMMON_H



void debugf(const char* format,...);


void perrorf(const char* format,...);

int recv_bytes(int sk, char** buf, int *length);
int send_bytes(int sk, char* buf, int len);



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

    uint32_t mss_clamp;
    uint32_t snd_wscale;
    uint32_t rcv_wscale;
    uint16_t has_rcv_wscale;

};


int con_info_serialize(char **buf, int *len, const struct con_info_type *con_info);
int con_info_deserialize(struct con_info_type *con_info, const char *buf, int len);

struct con_info_reply{
    uint8_t is_leader;
    uint32_t isn;
};

void print_con (struct con_id_type *con_id, const char* format,...);

#define MUTEX_LOCK


#if defined(SPIN_LOCK)
#define LOCK(x) pthread_spin_lock(&x)
#define UNLOCK(x) pthread_spin_unlock(&x);
#define LOCK_INIT(x) pthread_spin_init(&x, 0)
#define DECLARE_LOCK(x) pthread_spinlock_t x  
#elif defined(MUTEX_LOCK)
#define LOCK(x) pthread_mutex_lock(&x)
#define UNLOCK(x) pthread_mutex_unlock(&x)
#define LOCK_INIT(x) pthread_mutex_init(&x, NULL)
#define DECLARE_LOCK(x) pthread_mutex_t x
#endif


#endif // COMMON_H