#include <stdio.h>
#include <stdlib.h>

#include "../../src/include/packet-buffer/packet-buffer.h"
#include <stdint.h>
#include <pthread.h>

#define RINGBUF_SIZE 4096

void *test_1(void *arg){
	char* stra ="abcdefg";

	while(1){
		write_to_packet_buffer(stra, 8);
		printf("[insert] abcdefg\n");
	}
}

void *test_2(void *arg){
	char* strb ="uvwxyz";

	while(1){
		write_to_packet_buffer(strb, 7);
		printf("[insert] uvwxyz\n");
	}
}


void *test_3(void *arg){
	char *buffer;
	while(1){
		int len;
		len =read_from_packet_buffer(&buffer);
		if (len != 0)
			{
				printf("[Read]: %s\n", buffer);
				free(buffer);
			}
	}
}


int main(int argc, char** argv){
	init_packet_buffer();
	int i =0 ;
	pthread_t tid; 
	pthread_create(&tid, NULL, &test_3, NULL);

	pthread_create(&tid, NULL, &test_1, NULL);
	pthread_create(&tid, NULL, &test_2, NULL);
	pthread_join(tid, NULL);
	return 0;
}