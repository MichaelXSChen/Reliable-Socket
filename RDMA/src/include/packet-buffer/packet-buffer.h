#ifndef PACKET_BUFFER_H
#define PACKET_BUFFER_H
#include <stdint.h>

void init_packet_buffer();

int read_from_packet_buffer(uint8_t **buffer);


int write_to_packet_buffer(const uint8_t *buffer, size_t len);

#endif