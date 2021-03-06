#include <pthread.h>

#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/uthash.h"

#include <unistd.h>

#define REPORT_PORT 6666
#define QUERY_PORT 7777
#define BUFLEN 512
#define MY_HOST_IP "10.22.1.2"
#define CON_MGR_PORT 4321
#define HB_PORT 6667

//#define DEBUG_BAKCUP
typedef enum { false, true } bool;



static bool iamleader;
static bool sk_consensus_connected;


static int sk_ask_consensus; 
static int sk_con_info;
static int sk_create_connection;
static int sk_listen;
static int sk_recv_hb;



struct con_list_key {
	uint32_t dst_ip;
	uint16_t dst_port; 
};



struct con_list_entry{
	struct con_list_key key;
	uint32_t src_ip;
	uint16_t src_port;

    uint32_t send_seq; 
    uint32_t recv_seq;
    uint16_t has_timestamp;
    uint32_t timestamp;

    uint32_t mss_clamp;
    uint32_t snd_wscale;
    uint32_t rcv_wscale;
    uint16_t has_rcv_wscale;
    
    UT_hash_handle hh;
};

typedef struct con_list_entry con_list_entry;


con_list_entry *connect_con_list; 

pthread_spinlock_t hash_lock;



static int hash_insert(struct con_info_type *con_info){
	con_list_entry *entry = (con_list_entry *)malloc(sizeof(con_list_entry));
	
	print_con(&(con_info->con_id), "Insert into db");

	memset(entry, 0, sizeof(con_list_entry));
	

	entry->key.dst_ip = con_info->con_id.dst_ip;
	entry->key.dst_port = con_info->con_id.dst_port;


	entry->src_ip = con_info->con_id.src_ip;
	entry->src_port = con_info->con_id.src_port;


	entry->send_seq = con_info->send_seq;
	entry->recv_seq = con_info->recv_seq;
	entry->has_timestamp = con_info->has_timestamp;
	entry->timestamp = con_info->timestamp;
	entry->mss_clamp = con_info->mss_clamp;
	entry->snd_wscale = con_info->snd_wscale;
	entry->rcv_wscale = con_info->rcv_wscale;
	entry->has_rcv_wscale = con_info->has_rcv_wscale;

	struct con_list_entry *replaced;




	pthread_spin_lock(&hash_lock);
	HASH_REPLACE(hh, connect_con_list, key, sizeof(struct con_list_key), entry, replaced);
	pthread_spin_unlock(&hash_lock);




	return 0;
}


static int hash_find (struct con_info_type *con_info){
	
	print_con(&(con_info->con_id), "Searching");


	struct con_list_key *key = (struct con_list_key*)malloc(sizeof(struct con_list_key));
	memset(key, 0, sizeof(struct con_list_key));

	key->dst_port = con_info->con_id.dst_port;
	key->dst_ip = con_info->con_id.dst_ip;


	con_list_entry *entry;


	pthread_spin_lock(&hash_lock);

	HASH_FIND(hh, connect_con_list, key, sizeof(struct con_list_key), entry);
	pthread_spin_unlock(&hash_lock);


	if (entry == NULL){
		debugf("Search Failed");
		return -1;
	}
	else{
		con_info->con_id.src_ip = entry->src_ip;
		con_info->con_id.src_port = entry->src_port;


		con_info->send_seq = entry->send_seq;
		con_info->recv_seq = entry->recv_seq;
		con_info->has_timestamp = entry->has_timestamp;
		con_info->timestamp = entry->timestamp;
		con_info->mss_clamp = entry->mss_clamp;
		con_info->snd_wscale = entry->snd_wscale;
		con_info->rcv_wscale = entry->rcv_wscale;
		con_info->has_rcv_wscale = entry->has_rcv_wscale;

		return 0;
	}
}





void create_connection(struct con_info_type *con_info){
	int ret;


	
	//create connection;
	sk_create_connection = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sk_create_connection < 0){
		perror("Can not create socket");
		close(sk_create_connection);
		pthread_exit(0);
	}
	debugf("SK created for creating connection: sk_create_connection = %d", sk_create_connection);

	struct sockaddr_in addr; 
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = con_info->con_id.src_ip;
	addr.sin_port = con_info->con_id.src_port;


	int count = 1;
	ret = -1;

	
	
	while (ret < 0 && count < 11){


		debugf("[%d-th Try]: Creating connection to address %s:%d", count, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		ret = connect(sk_create_connection, (struct sockaddr *)&addr, sizeof(addr));
		
		if (ret <0){
			perror("Can not connect");
			//close(sk_create_connection);
			//pthread_exit(0);
		}
		count++;
		sleep(1);
	}

	if (ret < 0){
		perror("Cannnot connect after 10 tries");
		close(sk_create_connection);
		pthread_exit(0);
	}

	
	int len;
	char* buffer;
	con_info_serialize(&buffer, &len, con_info);

	ret = send_bytes(sk_create_connection, buffer, len);
	if (ret < 0){
		perrorf("failed to send");
	}


	//close(sk_create_connection);
	free(buffer);

	return;

}


void * serve_report(void * arg){
	int recv_len;
	char buf[BUFLEN];
	struct sockaddr_in si_other;
	int fromlen = sizeof(si_other);



	struct in_addr host_addr;
	int ret = inet_aton(MY_HOST_IP, &host_addr);
	uint64_t addr = host_addr.s_addr;





	while(1)
	{
		recv_len = recvfrom(sk_con_info, buf, BUFLEN, 0, (struct sockaddr*)&si_other , &fromlen);

		if( si_other.sin_addr.s_addr !=  addr ){
			continue;
		}


		debugf("[Con-Manager] Received consensused connection of lenth %d", recv_len);
		if (iamleader == false){
			struct con_info_type *con_info;
			con_info = (struct con_info_type *)malloc(sizeof(struct con_info_type));
			con_info_deserialize(con_info, buf, recv_len);
			
			debugf("\n\n Type : %"PRIu16 "\n\n", con_info->con_type);
			if(con_info->con_type == ACCEPT_CON){
				create_connection(con_info);
			}
			else if(con_info->con_type == CONNECT_CON){
				debugf("Received Info for connection create by connect, will save it into the hashmap");
				hash_insert(con_info);
			}

		}
	}

}


int sk_ask_consensus_connect(){

	int ret;
	struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    ret = inet_aton(MY_HOST_IP, &addr.sin_addr);
    if (ret < 0) {
        perror("Can't convert addr");
        return -1;
    }
    addr.sin_port = htons(CON_MGR_PORT);

    ret = connect(sk_ask_consensus, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("Can't connect");
        return -1;
    }
    debugf("[Con-Manager] socket for asking for consensus connected !");
    sk_consensus_connected =true;


    return 0;
}



int send_for_consensus(struct con_info_type *con_info){
	
    int len, ret; 
    char* buffer;
	ret = con_info_serialize(&buffer, &len, con_info);

	if (sk_consensus_connected == false){
		ret = sk_ask_consensus_connect();
		if (ret < 0){
			perrorf("failed to connect to qemu");
			return -1;
		}
	}
	
	ret = send_bytes(sk_ask_consensus, buffer, len);

	if (ret < 0) {


		if(errno == ECONNRESET || errno == EDESTADDRREQ){
			int tmp_errno = errno;
			ret = sk_ask_consensus_connect();
			if (ret != 0){
				perrorf("Failed to send due to :%s\n, Failed to connect: ", strerror(tmp_errno));
				return -1;
			}
			ret = send_bytes(sk_ask_consensus, buffer, len);
			if (ret != 0){
				perrorf("Failed to send the first time :%s\n, Failed to send the second time", strerror(tmp_errno));
				return -1;
			}
			debugf("[Con-Manager] Reconnected and sent, previous error was: %s", strerror(tmp_errno));
			return 0;
		}
		else{
	        perror("[Con-Manager] Send for consensus failed");
	        return -1;
	    }
    }

	return 0;
}



void *serve_query(void *sk_arg){
	int sk = *((int *) sk_arg);
	while(1){
		char *buf;
		int ret, len;
		ret = recv_bytes(sk, &buf, &len);
		if (ret < 0){
			//perror("Failed to recv bytes");
			close(sk);
			pthread_exit(0);
		}
		if (ret == 0){
			close(sk);
			pthread_exit(0);
		}
		if (len == 1){
			uint8_t leader = (uint8_t)iamleader; 
			debugf("[Con-Manager]: LD_PRELOAD module is checking for leadership, I am ? %"PRIu8"", leader);

			ret = send_bytes(sk, (char*)&leader, 1);
			continue;
		}


		struct con_info_type con_info;
		ret = con_info_deserialize(&con_info, buf, len);
		
		if(iamleader == true){

			ret = send_for_consensus(&con_info);
			if (ret < 0){
				perrorf("Faild to Send for consensus");
				pthread_exit(0);
			}
			debugf("[Con-Manager] Successfully sent connection info to QEMU for consensus");

			ret = send(sk, &iamleader, sizeof(iamleader), 0);
			// close(*sk);
			// pthread_exit(0);
			continue;

		}
		else if (con_info.con_type == CONNECT_QUERY) {//
			ret = hash_find(&con_info);
			if (ret == -1){
				//NO Info for now, return 0;
				uint8_t has_info = 0;
				ret = send(sk, &has_info, sizeof(has_info), 0);
				if (ret <= 0){
					perrorf("Failed to send success message back to LD_PRELOAD module");
				}
			}
			else{
				//Found info 

				uint8_t has_info = 1;
				ret = send(sk, &has_info, sizeof(has_info), 0);
				if (ret <= 0){
					perrorf("Failed to send success message back to LD_PRELOAD module");
				}
				int tmp = sk;

				char *out_buf; 
				int out_len;
				ret = con_info_serialize(&out_buf, &out_len, &con_info);	// This lib seems to have memory leak. 
				
				sk = tmp;

				ret = send_bytes(sk, out_buf, out_len);

				if (ret < 0){
					perrorf("Failed to send connection info back to LD_PRELOAD module");
				}

			}
			//continue;
		}
		else{
			continue;
		}
	}	
}


void *listen_for_accpet(){
	
	int ret;
	while(1){
		struct sockaddr_in cliaddr;
		socklen_t clilen=sizeof(cliaddr);
		int ask = accept(sk_listen, (struct sockaddr*)&cliaddr, &clilen);
		if (ask < 0){
			perror("[Con-Manager] Cannot accept new con");
		}
		pthread_t thread1;
		ret = pthread_create(&thread1, NULL, &serve_query, (void *)&ask);
		if (ret < 0){
			perror("[Con-Manager] Faild to create");
			pthread_exit(0);
		}
	}
}


void *recv_hb(void *useless){

	int recv_len;
	int buflen = 32;
	char buf[buflen];
	struct in_addr host_addr;
	int ret = inet_aton(MY_HOST_IP, &host_addr);
	uint64_t addr = host_addr.s_addr;


	if (ret == 0){
		perrorf("[Con-Manager] Wrong host ip foramt");
		exit(1);
	}

	debugf("[Con-Manager] RECV HB thread working");
	while(1)
	{
		recv_len = recvfrom(sk_recv_hb, buf, buflen, 0, NULL ,NULL);
		if (*(uint64_t*)buf == addr){
			iamleader = true;
		}
		else{
			//TODO: Currently not support a non-crash leader becomes a backup. 
			// if (iamleader == true)
			// 	debugf("[Con-Manager] I am no longer leader");
			iamleader = false;
		}
	}
}


int init_con_manager_guest(){
	connect_con_list = NULL;
	pthread_spin_init(&hash_lock, 0);


	iamleader = false;



	sk_ask_consensus = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sk_ask_consensus < 0) {
        perror("[Con-Manager] Can't create socket");
        return -1;
    }
    sk_consensus_connected = false;

	sk_con_info = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sk_con_info <= 0){
		perror("[Con-Manager] Failed to create socket");
		exit(1);
	}
	struct sockaddr_in si_con_info;
	memset(&si_con_info, 0, sizeof(si_con_info));

	si_con_info.sin_family = AF_INET;
	si_con_info.sin_port = htons(REPORT_PORT);
	si_con_info.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sk_con_info, (struct sockaddr*)&si_con_info, sizeof(si_con_info)) == -1){
		perror("[Con-Manager] Failed to bind to address");
		exit(1);
	}
	debugf("[Con-Manager] [SK-%d]: Listening for consensused connection info on port: %d",sk_con_info, REPORT_PORT);


	pthread_t report_thread;
	int ret;
	ret = pthread_create(&report_thread, NULL, serve_report, NULL);
	if (ret != 0){
		perror("[Con-Manager] Failed to create pthred");
		exit(1);
	} 



	sk_listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sk_listen < 0){
		perror("[Con-Manager] Cannot create the socket");
		return -1;
	}


	struct sockaddr_in srvaddr;

	memset(&srvaddr, 0, sizeof(srvaddr));

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(QUERY_PORT); 

	debugf("[Con-Manager] [SK-%d]: Listening to query from LD_PRELOAD module on port: %d", sk_listen, QUERY_PORT);

	ret = bind(sk_listen, (struct sockaddr*)&srvaddr, sizeof(srvaddr));
	if (ret < 0){
		perror("[Con-Manager] Cannot bind to sk");
		return -1;
	}

	ret = listen(sk_listen, SOMAXCONN);
	if (ret < 0){
		perror("[Con-Manager] Failed to put sock into listen state");
		return -1;
	}

	pthread_t listen_thread;
	pthread_create(&listen_thread, NULL, listen_for_accpet, NULL);

	sk_recv_hb = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sk_recv_hb <= 0){
		perror("[Con-Manager] Failed to create socket to recv heartbeat");
		return -1;
	}

	struct sockaddr_in si_hb;
	memset(&si_hb, 0, sizeof(si_hb));

	si_hb.sin_family = AF_INET;
	si_hb.sin_port = htons(HB_PORT);
	si_hb.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sk_recv_hb, (struct sockaddr*)&si_hb, sizeof(si_hb)) == -1){
		perror("[Con-Manager] Failed to bind to address");
		exit(1);
	}
	debugf("[Con-Manager] [SK-%d]: Listening for heartbeat on port: %d",sk_recv_hb, HB_PORT);




	pthread_t recv_hb_thread;
	pthread_create(&recv_hb_thread, NULL, recv_hb, NULL);


	return 0;
}


