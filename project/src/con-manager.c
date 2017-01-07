#include "include/common.h"
#include "include/uthash.h"
#include "include/util.h"

#include <pthread.h>
#include <stdint.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>

#define SERVICE_PORT 4321

int sk;

struct con_list_entry{
	struct con_id_type con_id;
    uint32_t isn; 
    UT_hash_handle hh;
};

typedef struct con_list_entry con_list_entry;
con_list_entry *con_list;



int find(con_list_entry **entry ,struct con_id_type *con){
	//debug("Finding entry with src_ip:%" PRIu32", src_port:%"PRIu16", dst_ip:%"PRIu32", dst_port:%"PRIu16"", con->src_ip, con->src_port, con->dst_ip, con->dst_port);
	HASH_FIND(hh, con_list, con, sizeof(struct con_id_type), *entry);
	return 0;
}

int insert(con_list_entry* entry){
	struct con_list_entry *tmp = NULL; 
	find(&tmp, &(entry->con_id));
	if (tmp == NULL){
		HASH_ADD(hh, con_list, con_id, sizeof(struct con_id_type), entry);
	//	debug("adding entry with src_ip=%"PRIu32", isn = %"PRIu32"", entry->con_id.src_ip, entry->isn);
	//	debug("\n\n Size of Hashtable: %d", HASH_COUNT(con_list));
	}
	else {
		//TODO: How to do now;

		debug("conflict, isn = %"PRIu32"", tmp->isn);
	}
	return 0;
}



int iamleader(){
	//TODO:return 1 if it is leader, 0 other wise. 
	//The manager will work differently if is leader or not
	return 0;
}



int handle_con_info(){
	return 0;
}


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
		

		debug("SRC_ADDR: %" PRIu32 "",con_info.con_id.src_ip);
		debug("SRC_PORT: %" PRIu16 "",con_info.con_id.src_port);
		debug("DST_ADDR: %" PRIu32 "",con_info.con_id.dst_ip);
		debug("DST_PORT: %" PRIu16 "",con_info.con_id.dst_port);
		debug("ISN: %" PRIu32 "", con_info.isn);

		uint8_t is_leader;
		if (iamleader()){
			//TODO: Make consensus 

			/**
			Consensus 
			**/

			//
			is_leader = 1;
			ret = send(*sk, &is_leader, sizeof(is_leader), 0);
			if (ret < 0){
				printf("Failed to send reply back");
				pthread_exit(0);
			}
		}else{
			//Get the entry
			//return the entry
			con_list_entry *entry=NULL; 
			find(&entry,&(con_info.con_id));
			while(entry == NULL){
				debug("NO match, try again");
				sleep(1);
				find(&entry,&(con_info.con_id));
			}
			debug("Match, isn = %"PRIu32"", entry->isn);
			is_leader = 0;
			ret = send(*sk, &is_leader, sizeof(is_leader), 0);
			if (ret < 0){
				printf("Failed to send reply back");
				pthread_exit(0);
			}

			con_info.isn = entry->isn;
			int len;
			char* buf;
			ret = con_info_serialize(&buf, &len, &con_info);

			ret = send_bytes(*sk, buf, len);

		    if (ret < 0){
		        perror("Failed to send con_info");
		        pthread_exit(0);
		    }

		}	




		close(*sk);
		pthread_exit(0);
	}	
}




int init(){
	// Init the hash table
	con_list = NULL;
	int ret;

	con_list_entry *entry = (con_list_entry *)malloc(sizeof(con_list_entry));
	entry->con_id.src_port = 1122;
	entry->con_id.src_ip = 12345;
	entry->con_id.dst_ip = 67890;
	entry->con_id.dst_port = 2211;
	entry->isn = 950506;

	insert(entry);
	insert(entry);







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




int main(){
	init();
}