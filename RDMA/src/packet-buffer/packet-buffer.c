#include "../../utils/ringbuf/ringbuf.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define RINGBUF_SIZE 65536

ringbuf_t rb;

pthread_mutex_t lock;

void init_packet_buffer(){
	rb = ringbuf_new(RINGBUF_SIZE);
	pthread_mutex_init(&lock, NULL);
}

int read_from_packet_buffer(uint8_t **buffer){
	pthread_mutex_lock(&lock);
	size_t len = ringbuf_bytes_used(rb);
	if (len <= 0){
		pthread_mutex_unlock(&lock);
		return 0;
	}
	*buffer =(uint8_t*)malloc(len);
	ringbuf_memcpy_from(*buffer, rb, len);
	pthread_mutex_unlock(&lock);
	usleep(100);
	return len;
}

int write_to_packet_buffer(const uint8_t *buffer, size_t len){
	pthread_mutex_lock(&lock);
	ringbuf_memcpy_into(rb, buffer, len);
	pthread_mutex_unlock(&lock);
	usleep(100);

	return len;
}

int packet_buffer_to_buffer(uint8_t *buffer, int maxlen){
	pthread_mutex_lock(&lock);
	int len =(int)ringbuf_bytes_used(rb);
	if (len <= 0){
		pthread_mutex_unlock(&lock);
		return 0;
	}
	int read_len = (len < maxlen) ? len : maxlen;
	ringbuf_memcpy_from(*buffer, rb, read_len);
	pthread_mutex_unlock(&lock);
	return read_len;
}