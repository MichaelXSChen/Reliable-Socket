#include "include/common.h"
#include "include/uthash.h"
#include "include/con-manager.h"
#include <pthread.h>
#include <stdint.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>



int sk;


con_list_entry *con_list;



int find_connection(con_list_entry **entry ,struct con_id_type *con){
	//debugf("Finding entry with src_ip:%" PRIu32", src_port:%"PRIu16", dst_ip:%"PRIu32", dst_port:%"PRIu16"", con->src_ip, con->src_port, con->dst_ip, con->dst_port);
	HASH_FIND(hh, con_list, con, sizeof(struct con_id_type), *entry);
	return 0;
}

int insert_connection(con_list_entry* entry){
	struct con_list_entry *tmp = NULL; 
	find_connection(&tmp, &(entry->con_id));
	if (tmp == NULL){
		HASH_ADD(hh, con_list, con_id, sizeof(struct con_id_type), entry);
	//	debugf("adding entry with src_ip=%"PRIu32", isn = %"PRIu32"", entry->con_id.src_ip, entry->isn);
	//	debugf("\n\n Size of Hashtable: %d", HASH_COUNT(con_list));
	}
	else {
		//TODO: How to do now;

		debugf("conflict, isn = %"PRIu32"", tmp->isn);
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
	debugf("sk: %d", *sk);
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
		

		debugf("SRC_ADDR: %" PRIu32 "",con_info.con_id.src_ip);
		debugf("SRC_PORT: %" PRIu16 "",con_info.con_id.src_port);
		debugf("DST_ADDR: %" PRIu32 "",con_info.con_id.dst_ip);
		debugf("DST_PORT: %" PRIu16 "",con_info.con_id.dst_port);
		debugf("ISN: %" PRIu32 "", con_info.isn);

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
			find_connection(&entry,&(con_info.con_id));
			while(entry == NULL){
				debugf("NO match, try again");
				sleep(1);
				find_connection(&entry,&(con_info.con_id));
			}
			debugf("Match, isn = %"PRIu32"", entry->isn);
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




int con_manager_init(){
	// Init the hash table
	con_list = NULL;
	int ret;

	con_list_entry *entry = (con_list_entry *)malloc(sizeof(con_list_entry));
	entry->con_id.src_port = htons(9999);
	entry->con_id.src_ip = 16777343;
	entry->con_id.dst_ip = 16777343;
	entry->con_id.dst_port = htons(10060);
	entry->isn = 931209;

	insert_connection(entry);







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

	debugf("binding to port: %d", SERVICE_PORT);

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
	con_manager_init();
}
