#include "../../utils/ringbuf/ringbuf.h"
#include "../include/vpb/common.h"
#include "../include/packet-buffer/packet-buffer.h"
#include "../include/vpb/con-manager.h"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define OUTGOING_BUFFER_SIZE 5120000000
#define TCP_BUFFER_SIZE 5120000000


static ringbuf_t outgoing_buffer;
static ringbuf_t tcp_buffer;

#define USE_SPINLOCK //It shows that spin lock is faster than mutex

static pthread_spinlock_t outgoing_buffer_lock;
static pthread_spinlock_t tcp_buffer_lock;

/**************
PACKET Buffer Format

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
/****************
*TCP buffer format. 
* CON_ID, ACK, Packet Length, Packet
********************/


int write_to_tcp_buffer(struct con_id_type *con_id, uint32_t ack,  const uint8_t *buffer, size_t len){
	pthread_spin_lock(&tcp_buffer_lock);
	

	ringbuf_memcpy_into(tcp_buffer, con_id, sizeof(struct con_id_type));
	


	ringbuf_memcpy_into(tcp_buffer, &ack, sizeof(ack));	

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
	
	//Make compare using head pointer. 
	struct con_id_type *con_id_ptr = (struct con_id_type *)ringbuf_tail(tcp_buffer);
	uint32_t *ack_ptr = (uint32_t *)(ringbuf_tail(tcp_buffer)+sizeof(struct con_id_type));
	// uint32_t outgoing_seq; 
	// get_con_out_seq(&outgoing_seq, con_id_ptr);



	int ret= check_block(*ack_ptr, con_id_ptr);
	
	if (ret == 1){
		pthread_spin_unlock(&tcp_buffer_lock);
		pthread_spin_unlock(&outgoing_buffer_lock);
		return -1;
	}



	struct con_id_type con_id; 
	ringbuf_memcpy_from(&con_id, tcp_buffer, sizeof(struct con_id_type));
	uint32_t ack;
	ringbuf_memcpy_from(&ack, tcp_buffer, sizeof(ack));



	size_t next_buffer_len;

	
	ringbuf_memcpy_from(&next_buffer_len, tcp_buffer, sizeof(next_buffer_len));
	
	uint8_t *buffer = (uint8_t *)malloc(next_buffer_len);
	
	ringbuf_memcpy_from(buffer, tcp_buffer, next_buffer_len);



	ringbuf_memcpy_into(outgoing_buffer, &next_buffer_len, sizeof(next_buffer_len));
	ringbuf_memcpy_into(outgoing_buffer, buffer, next_buffer_len);



	pthread_spin_unlock(&tcp_buffer_lock);
	pthread_spin_unlock(&outgoing_buffer_lock);


	return next_buffer_len;
}

