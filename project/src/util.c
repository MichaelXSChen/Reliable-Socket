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
	
	*buf = (char *)malloc(*length); 
	char* data= *buf;


	//memset(*buf, 1, *length);
	int left = *length;
	int ret = 0;
	do{
		ret = recv(sk, data, left, 0);
		if (ret < 0){
			//TODO: Deal with errors;
			perrorf("Error recving request");
			return -1;
		}
		else{
			data += ret;
			left -= ret;
		}
	}while(left >0);



	debug("recv bytes with length: %d", *length);
	return 0;
}





int send_bytes(int sk, char* buf, int len){
	// int32_t tmp = htonl(argc);
	int32_t length = htonl(len);
	int ret; 
	int left = len + sizeof(length);
	char* data = (char*) malloc(left);
	memcpy(data, &length, sizeof(length));
	// memcpy(&data[sizeof(length)], &tmp, sizeof(tmp));
	memcpy(&data[sizeof(length)], buf, len);

	
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
	}while(left >0);

	debug("Sent bytes with len: %d", len);
	return 0;
}
