#include "../../utils/ringbuf/ringbuf.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define OUTGOING_BUFFER_SIZE 16777216
#define TCP_BUFFER_SIZE 16777216


static ringbuf_t outgoing_buffer;
static ringbuf_t tcp_buffer;

#define USE_SPINLOCK //It shows that spin lock is faster than mutex

static pthread_spinlock_t outgoing_buffer_lock;
static pthread_spinlock_t tcp_buffer_lock;

/**************
Buffer Format

....|size_t len_of_buffer| buffer contents |..... 

*****************/
void init_packet_buffer(){
	outgoing_buffer = ringbuf_new(OUTGOING_BUFFER_SIZE);
	tcp_buffer = ringbuf_new(TCP_BUFFER_SIZE);

	pthread_spin_init(&tcp_buffer_lock, 0);
	pthread_spin_init(&outgoing_buffer_lock, 0);

}

int read_from_packet_buffer(uint8_t **buffer){

	pthread_spin_lock(&outgoing_buffer_lock);
	size_t len = ringbuf_bytes_used(outgoing_buffer);
	if (len <= 0){
		pthread_spin_unlock(&outgoing_buffer_lock);
		return 0;
	}
	size_t buffer_len; 
	//Get lenght of next packet
	ringbuf_memcpy_from(&buffer_len, outgoing_buffer, sizeof(buffer_len));


	*buffer =(uint8_t*)malloc(buffer_len);
	ringbuf_memcpy_from(*buffer, outgoing_buffer, buffer_len);

	pthread_spin_unlock(&outgoing_buffer_lock);
	return buffer_len;
}

int write_to_packet_buffer(const uint8_t *buffer, size_t len){
	pthread_spin_lock(&outgoing_buffer_lock);

	//Write the length first
	ringbuf_memcpy_into(outgoing_buffer, &len, sizeof(len));


	ringbuf_memcpy_into(outgoing_buffer, buffer, len);

	pthread_spin_unlock(&outgoing_buffer_lock);

	return len;
}

int packet_buffer_to_buffer(uint8_t *buffer, int maxlen){
	//TODO: Currently ignore max len

	pthread_spin_lock(&outgoing_buffer_lock);


	int len =(int)ringbuf_bytes_used(outgoing_buffer);
	if (len <= 0){

		pthread_spin_unlock(&outgoing_buffer_lock);
		return 0;
	}

	size_t next_buffer_len;
	ringbuf_memcpy_from(&next_buffer_len, outgoing_buffer, sizeof(next_buffer_len));


	//int read_len = (len < maxlen) ? len : maxlen;
	ringbuf_memcpy_from(buffer, outgoing_buffer, next_buffer_len);
	pthread_spin_unlock(&outgoing_buffer_lock);

	return next_buffer_len;
}

int write_to_tcp_buffer(const uint8_t *buffer, size_t len){
	pthread_spin_lock(&tcp_buffer_lock);
	ringbuf_memcpy_into(tcp_buffer, &len, sizeof(len));
	ringbuf_memcpy_into(tcp_buffer, buffer, len);
	pthread_spin_unlock(&tcp_buffer_lock);
	return len;
}





int dump_tcp_buffer(){
	pthread_spin_lock(&outgoing_buffer_lock);
	pthread_spin_lock(&tcp_buffer_lock);

	size_t len = ringbuf_bytes_used(tcp_buffer);
	if (len <= 0){
		pthread_spin_unlock(&tcp_buffer_lock);
		pthread_spin_unlock(&outgoing_buffer_lock);
		return 0;
	}
	

	uint8_t *buffer =(uint8_t*)malloc(len);
	ringbuf_memcpy_from(buffer, tcp_buffer, len);
	

	ringbuf_memcpy_into(outgoing_buffer, buffer, len);



	pthread_spin_unlock(&tcp_buffer_lock);
	pthread_spin_unlock(&outgoing_buffer_lock);


	return len;
}

