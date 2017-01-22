#include <pthread.h>

#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/con-hashmap.h"
#include "unistd.h"

#define REPORT_PORT 6666
#define QUERY_PORT 7777
#define BUFLEN 512
#define MY_HOST_IP "10.22.1.2"
#define CON_MGR_PORT 4321
#define HB_PORT 6667

//#define DEBUG_BAKCUP
typedef enum { false, true } bool;



static bool iamleader;

static int sk_ask_consensus; 
static int sk_con_info;
static int sk_create_connection;
static int sk_listen;
static int sk_recv_hb;


void create_connection(struct con_info_type *con_info){
	int ret;

	//insert into hashmap first 
	con_list_entry entry;
	entry.con_id = con_info->con_id;
	entry.recv_seq = con_info->recv_seq;
	entry.send_seq = con_info->send_seq;
	

	// ret = insert_connection(&entry);
	// if (ret <0){
	// 	perrorf("Failed to insert into hashmap");
	// 	pthread_exit(0);
	// }

	
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

	debugf("Creating connection to address %s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	ret = connect(sk_create_connection, (struct sockaddr *)&addr, sizeof(addr));
	if (ret <0){
		perror("Can not connect");
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
	free(buffer);

	return;

}


void * serve_report(void * arg){
	int recv_len;
	char buf[BUFLEN];
	struct sockaddr_in si_other;
	int fromlen = sizeof(si_other);
	while(1)
	{
		recv_len = recvfrom(sk_con_info, buf, BUFLEN, 0, (struct sockaddr*)&si_other , &fromlen);
		// if (recv_len != CON_INFO_SERIAL_LEN){
		// 	continue;
		// }

		debugf("Report Port received packet of lenth %d", recv_len);
		if (iamleader == false){
			struct con_info_type *con_info;
			con_info = (struct con_info_type *)malloc(sizeof(struct con_info_type));
			con_info_deserialize(con_info, buf, recv_len);
			create_connection(con_info);
		}
		//insert_connection_bytes(buf, recv_len);
	}

}





int send_for_consensus(struct con_info_type *con_info){
	
    int len, ret; 
    char* buffer;
	ret = con_info_serialize(&buffer, &len, con_info);

	ret = send_bytes(sk_ask_consensus, buffer, len);
	if (ret < 0) {
        perror("Send for consensus failed");
        return -1;
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
			perror("Failed to recv bytes");
			continue;
		}
		if (ret == 0){
			close(sk);
			pthread_exit(0);
		}
		if (len == 1){
			ret = send_bytes(sk, (char*)&iamleader, 1);
			// close(*sk);
			// pthread_exit(0);
			continue;
		}


		struct con_info_type con_info;
		ret = con_info_deserialize(&con_info, buf, len);
		

		debugf("SRC_ADDR: %" PRIu32 "",con_info.con_id.src_ip);
		debugf("SRC_PORT: %" PRIu16 "",con_info.con_id.src_port);
		debugf("DST_ADDR: %" PRIu32 "",con_info.con_id.dst_ip);
		debugf("DST_PORT: %" PRIu16 "",con_info.con_id.dst_port);
		debugf("SEND_SEQ: %" PRIu32 "", con_info.send_seq);
		debugf("RECV_SEQ: %" PRIu32 "", con_info.recv_seq);

		if(iamleader){
			send_for_consensus(&con_info);
			ret = send(sk, &iamleader, sizeof(iamleader), 0);
			// close(*sk);
			// pthread_exit(0);
			continue;

		}
		else{
			continue;
		}
	}	
}


// int check_for_leadership(){
// 	int ret;
// 	sk_ask_consensus = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
//     if (sk_ask_consensus < 0) {
//         perror("Can't create socket");
//         return -1;
//     }
//     struct sockaddr_in addr;
//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     ret = inet_aton(MY_HOST_IP, &addr.sin_addr);
//     if (ret < 0) {
//         perror("Can't convert addr");
//         return -1;
//     }
//     addr.sin_port = htons(CON_MGR_PORT);

//     ret = connect(sk_ask_consensus, (struct sockaddr *)&addr, sizeof(addr));
//     if (ret < 0) {
//         perror("Can't connect");
//         return 0;
//     }

//     uint8_t iamleader = 0;

//     ret = send_bytes(sk_ask_consensus, &iamleader, 1);
//     if (ret < 0){
//     	return 0;
//     }
//     ret = recv(sk_ask_consensus, &iamleader, sizeof(iamleader), 0);
//     if (ret < 0){
//     	perror("Faield to recv response!");
//     	return -1;
//     }
   
//     return iamleader;
// }

void *listen_for_accpet(){
	
	int ret;
	while(1){
		struct sockaddr_in cliaddr;
		socklen_t clilen=sizeof(cliaddr);
		int ask = accept(sk_listen, (struct sockaddr*)&cliaddr, &clilen);
		if (ask < 0){
			perror("Cannot accept new con");
		}
		pthread_t thread1;
		ret = pthread_create(&thread1, NULL, &serve_query, (void *)&ask);
		if (ret < 0){
			perror("Faild to create");
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
		perrorf("Wrong host ip foramt");
		exit(1);
	}

	debugf("RECV HB thread working");
	while(1)
	{
		recv_len = recvfrom(sk_recv_hb, buf, buflen, 0, NULL ,NULL);
		debugf("Received heartbeat of length %d, payload:%"PRIu64, recv_len, *(uint64_t*)buf);
		if (*(uint64_t*)buf == addr){
			iamleader = true;
			//debugf("I am leader");
		}
		else{
			
			if (iamleader)
				debugf("I am no longer leader");
			iamleader = false;
		}
	}
}


int init_con_manager_guest(){
	init_con_hashmap();
	iamleader = false;

// #ifndef DEBUG_BAKCUP
// 	iamleader = check_for_leadership();
// #endif
	// debugf("I am leader? %"PRIu8"", iamleader);
	//Create a UDP socket
	//Wait for consensused input
	sk_con_info = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sk_con_info <= 0){
		perror("Failed to create socket");
		exit(1);
	}
	struct sockaddr_in si_con_info;
	memset(&si_con_info, 0, sizeof(si_con_info));

	si_con_info.sin_family = AF_INET;
	si_con_info.sin_port = htons(REPORT_PORT);
	si_con_info.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sk_con_info, (struct sockaddr*)&si_con_info, sizeof(si_con_info)) == -1){
		perror("Failed to bind to address");
		exit(1);
	}
	debugf("[SK-%d]: Listening for consensused connection info on port: %d",sk_con_info, REPORT_PORT);


	pthread_t report_thread;
	int ret;
	ret = pthread_create(&report_thread, NULL, serve_report, NULL);
	if (ret != 0){
		perror("Failed to create pthred");
		exit(1);
	} 



	sk_listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sk_listen < 0){
		perror("Cannot create the socket");
		return -1;
	}


	struct sockaddr_in srvaddr;

	memset(&srvaddr, 0, sizeof(srvaddr));

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(QUERY_PORT); 

	debugf("[SK-%d]: Listening to query from LD_PRELOAD module on port: %d", sk_listen, QUERY_PORT);

	ret = bind(sk_listen, (struct sockaddr*)&srvaddr, sizeof(srvaddr));
	if (ret < 0){
		perror("Cannot bind to sk");
		return -1;
	}

	ret = listen(sk_listen, 16);
	if (ret < 0){
		perror("Failed to put sock into listen state");
		return -1;
	}

	pthread_t listen_thread;
	pthread_create(&listen_thread, NULL, listen_for_accpet, NULL);

	sk_recv_hb = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sk_recv_hb <= 0){
		perror("Failed to create socket to recv heartbeat");
		return -1;
	}

	struct sockaddr_in si_hb;
	memset(&si_hb, 0, sizeof(si_hb));

	si_hb.sin_family = AF_INET;
	si_hb.sin_port = htons(HB_PORT);
	si_hb.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sk_recv_hb, (struct sockaddr*)&si_hb, sizeof(si_hb)) == -1){
		perror("Failed to bind to address");
		exit(1);
	}
	debugf("[SK-%d]: Listening for consensused connection info on port: %d",sk_recv_hb, REPORT_PORT);




	pthread_t recv_hb_thread;
	pthread_create(&recv_hb_thread, NULL, recv_hb, NULL);


	return 0;
}


