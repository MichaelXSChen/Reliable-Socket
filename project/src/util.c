#include "include/util.h"
#include "include/common.h"


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>


int recv_bytes(int sk, char** buf, int *length){
	int32_t tmp;
	recv(sk, &tmp, sizeof(tmp), 0);
	*length = ntohl(tmp);
	//debug("length %d", *length);
	*buf = (char *)malloc(*length); 
	char* data= *buf;


	//memset(*buf, 1, *length);
	int left = *length;
	int ret = 0;

	do{
		ret = recv(sk, data, left, 0);
		//debug("ret:%d", ret);
		if (ret < 0){
			//TODO: Deal with errors;
			perrorf("Error recving request");
			return -1;
		}
		else{
			data += ret;
			left -= ret;
		}
	}
	while(left >0);

	return 0;
}





int send_bytes(int sk, char* buf, int len){
	// int32_t tmp = htonl(argc);
	int32_t length = htonl(len);
	//debug("size of buf: %d", strlen(buf));
	//debug("length before htonl: %d", strlen(buf));
	int ret; 
	int left = strlen(buf) + sizeof(length);
	char* data = (char*) malloc(left);
	memcpy(data, &length, sizeof(length));
	// memcpy(&data[sizeof(length)], &tmp, sizeof(tmp));
	memcpy(&data[sizeof(length)], buf, strlen(buf));
	
	// int i; 
	// printf("\n\n\ndata\n");
	// fflush(stdout);
	// for (i=0; i<left; i++){
	// 	printf("%c", data[i]);
	// 	fflush(stdout);
	// }
	//debug("left: %d", left);
	
	do{
		ret = send(sk, data, left, 0);
		if (ret < 0){
			//TODO: Deal with error
			perrorf("Error sending request");
			return -1;
		}
		else
		{
			data += ret;
			left -= ret;
		}
	}
	while(left >0);
	return 0;
}
