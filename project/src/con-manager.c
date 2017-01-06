#include "include/common.h"
#include "include/uthash.h"
#include "include/util.h"

#include <pthread.h>
#include <stdint.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>


#define SERVICE_PORT 9999


int is_leader(){
	//TODO:return 1 if it is leader, 0 other wise. 
	//The manager will work differently if is leader or not
	return 0;
}

struct con_isn_pair_type *con_isn_list;
int sk; 

void *serve(void *sk_arg){
	int *sk = (int *) sk_arg;
	debug("sk: %d", *sk);
	while(1){
		char *buf;
		int ret, len;
		ret = recv_bytes(*sk, &buf, &len);
		if (ret < 0){
			perror("Failed to recv bytes");
			//TODO:exit??
		}
		struct con_info_type con_info;
		ret = con_info_deserialize(&con_info, buf, len);

		pthread_exit(0);
	}	
}




int init(){
	// Init the hash table
	//con_isn_pairs = NULL;
	int ret;


	sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sk < 0){
		perror("Cannot create the socket");
		return -1;
	}


	struct sockaddr_in srvaddr;

	memset(&srvaddr, 0, sizeof(srvaddr));

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(SERVICE_PORT); 

	debug("binding to port: %d", SERVICE_PORT);

	ret = bind(sk, (struct sockaddr*)&srvaddr, sizeof(srvaddr));
	if (ret < 0){
		perror("Cannot bind to sk");
		return -1;
	}

	ret = listen(sk, 16);
	if (ret < 0){
		perror("Failed to put sock into listen state");
		return -1;
	}
	while(1){
		struct sockaddr_in cliaddr;
		socklen_t clilen;
		int ask = accept(sk, (struct sockaddr*)&cliaddr, &clilen);
		if (ask < 0){
			perror("Cannot accept new con");
			return -1;
		}
		pthread_t thread1;
		ret = pthread_create(&thread1, NULL, &serve, (void *)&ask);
		if (ret < 0){
			perror("Failed to create thread");
			return -1;
		}
	}


	return 0;
}

static int insert(const struct con_isn_pair_type *pair){
	//TODO: Deal with conflict

	//HASH_ADD(hh, con_isn_list, con_id, sizeof(struct con_id_type), pair);
	return 0;
}


int main(){
	init();
}