#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H
#include <stdint.h>
#include "../vpb/common.h"

void init_packet_buffer();

int read_from_packet_buffer(uint8_t **buffer);

int packet_buffer_to_buffer(uint8_t *buffer, int maxlen);

int write_to_packet_buffer(const uint8_t *buffer, size_t len);

int write_to_tcp_buffer(struct con_id_type *con_id, uint32_t ack,  const uint8_t *buffer, size_t len);

int dump_tcp_buffer();

int append_no_op();
#endif
