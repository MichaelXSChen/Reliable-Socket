#include "../include/vpb/common.h"
#include "../../utils/uthash/uthash.h"
#include "../include/vpb/con-manager.h"
#include "../include/dare/dare_server.h"
#include "../include/proxy/proxy.h"

#include <pthread.h>
#include <stdint.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>



#define GUEST_IP "10.22.1.100"
#define REPORT_PORT 6666
#define HB_PORT 6667


#define MY_IP "10.22.1.2"


pthread_cond_t become_leader; 
pthread_mutex_t become_leader_lock;


static int sk;


static int report_sk;
// con_list_entry *con_list;

static int hb_sk;


typedef struct con_isn_entry con_isn_entry;



con_isn_entry *con_isn_list;

int get_isn(uint32_t *isn, struct con_id_type *con){
	con_isn_entry *entry;
	HASH_FIND(hh, con_isn_list, con, sizeof(struct con_id_type), entry);
	*isn = entry->isn; 
	return 0;
}

int save_isn(uint32_t isn, struct con_id_type *con){
	con_isn_entry *entry = (con_isn_entry *) malloc(sizeof(con_isn_entry));
	memcpy(&(entry->con_id), con, sizeof(struct con_id_type));
	entry->isn = isn; 
	struct con_isn_entry *replaced;


	debugf("trying to insert into hashtable");
	HASH_REPLACE(hh, con_isn_list, con_id, sizeof(struct con_id_type), entry, replaced);
	
	debugf("insert complete");
	return 0;
}


int report_to_guest_manager(char* buf, int len){
	struct sockaddr_in si;
	int slen= sizeof(si);
	
	memset(&si, 0, slen);
	si.sin_family=AF_INET;
	si.sin_port=htons(REPORT_PORT);
	inet_aton(GUEST_IP, &si.sin_addr);
	int send_len = sendto(report_sk, buf, len, 0, (struct sockaddr*)&si, slen);
	debugf("Sent to guest manager with len: %d", send_len);
	return len;

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
		if (ret == 0){
			close(*sk);
			pthread_exit(0);
		}

		uint8_t iamleader;
		if (len == 1){
			debugf("Check for leadership");
			iamleader = (uint8_t) is_leader();
			ret = send(*sk, &iamleader, sizeof(iamleader), 0);
			continue;
		}


		struct con_info_type con_info;
		ret = con_info_deserialize(&con_info, buf, len);
		

		debugf("SRC_ADDR: %" PRIu32 "",con_info.con_id.src_ip);
		debugf("SRC_PORT: %" PRIu16 "",con_info.con_id.src_port);
		debugf("DST_ADDR: %" PRIu32 "",con_info.con_id.dst_ip);
		debugf("DST_PORT: %" PRIu16 "",con_info.con_id.dst_port);
		debugf("SEND_SEQ: %" PRIu32 "", con_info.send_seq);
		debugf("RECV_DEQ: %" PRIu32 "", con_info.recv_seq);

		
		if (is_leader()){
			//TODO: Cleaner

			/**
			Consensus
			**/

			tcpnewcon_handle((uint8_t *)buf,len); 
			//
			
			// report_to_guest_manager(buf, len);


			iamleader = 1;
			ret = send(*sk, &iamleader, sizeof(iamleader), 0);
			

			if (ret < 0){
				printf("Failed to send reply back");
				pthread_exit(0);
			}
		}
		else{
			debugf("[ERROR] NO LEADER, shouldn't ask for consensus");

		}	

	}	
}


void *wait_for_connection(void *arg){
	int ret;
	while(1){
		struct sockaddr_in cliaddr;
		socklen_t clilen;
		int ask = accept(sk, (struct sockaddr*)&cliaddr, &clilen);
		if (ask < 0){
			perror("Cannot accept new con");
		}
		pthread_t thread1;
		ret = pthread_create(&thread1, NULL, &serve, (void *)&ask);
		if (ret < 0){
			pthread_exit(0);
		}
	}
}

void *hb_to_guest(void *arg){
	struct in_addr myaddr;
	memset(&myaddr, 0, sizeof(struct in_addr));
	inet_aton(MY_IP, &myaddr);
	uint64_t addr = myaddr.s_addr; 
	struct sockaddr_in si;
	int slen= sizeof(si);
	
	memset(&si, 0, slen);
	si.sin_family=AF_INET;
	si.sin_port=htons(HB_PORT);
	inet_aton(GUEST_IP, &si.sin_addr);


	while(1){
		if(is_leader()){
			 sendto(hb_sk, &addr, sizeof(addr), 0, (struct sockaddr*)&si, sizeof(si));
			 usleep(100);
		}	
		else{
			pthread_mutex_lock(&become_leader_lock);
            pthread_cond_wait(&become_leader, &become_leader_lock);
            pthread_mutex_unlock(&become_leader_lock);
		}
	}
}


int con_manager_init(){
	
	pthread_cond_init(&become_leader, NULL);
    pthread_mutex_init(&become_leader_lock, NULL);


	// Init the hash table

	con_isn_list = NULL;
	int ret;



	report_sk= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	debugf("socket to report to guest manager");
	if (report_sk< 0){
		perrorf("Failed to create report_sk");
		return -1;
	}

	hb_sk = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hb_sk < 0){
		perror("Failed to create hb_sk");
		return -1;
	}




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
	pthread_t listen_thread;

	ret = pthread_create(&listen_thread, NULL, &wait_for_connection, (void*)&sk);

	pthread_t hb_thread;
	ret = pthread_create(&hb_thread, NULL, &hb_to_guest, NULL);

	return 0;
}

int handle_consensused_con(char* buf, int len){
	struct con_info_type con_info; 
	memset(&con_info, 0, sizeof(con_info));

	int ret;
	ret = con_info_deserialize(&con_info, buf, len);

	uint32_t isn; 
	get_isn(&isn, &(con_info.con_id));


	con_info.recv_seq = isn; 


	char *buffer; 
	int length; 
	con_info_serialize(&buffer, &length, &con_info);
	report_to_guest_manager(buffer, length);

	return 0; 

}


