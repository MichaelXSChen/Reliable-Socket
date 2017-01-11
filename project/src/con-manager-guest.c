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


void * serve_report(void * arg){
	int sk = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sk <= 0){
		perror("Failed to create socket");
		exit(1);
	}
	struct sockaddr_in si_me;
	memset(&si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(REPORT_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (sk, (struct sockaddr*)&si_me, sizeof(si_me)) == -1){
		perror("Failed to bind to address");
		exit(1);
	}
	int recv_len;
	char buf[BUFLEN];
	struct sockaddr_in si_other;
	int fromlen = sizeof(si_other);
	while(1)
	{
		recv_len = recvfrom(sk, buf, BUFLEN, 0, (struct sockaddr*)&si_other , &fromlen);
		insert_connection_bytes(buf, recv_len);
	}

}



void *serve(void *sk_arg){
	int *sk = (int *) sk_arg;
	//debugf("sk: %d", *sk);
	while(1){
		char *buf;
		int ret, len;
		ret = recv_bytes(*sk, &buf, &len);
		if (ret < 0){
			perror("Failed to recv bytes");
			continue;
		}
		struct con_info_type con_info;
		ret = con_info_deserialize(&con_info, buf, len);
		

		debugf("SRC_ADDR: %" PRIu32 "",con_info.con_id.src_ip);
		debugf("SRC_PORT: %" PRIu16 "",con_info.con_id.src_port);
		debugf("DST_ADDR: %" PRIu32 "",con_info.con_id.dst_ip);
		debugf("DST_PORT: %" PRIu16 "",con_info.con_id.dst_port);
		debugf("ISN: %" PRIu32 "", con_info.isn);

		

		con_list_entry *entry=NULL; 
		find_connection(&entry,&(con_info.con_id));
		while(entry == NULL){
			debugf("NO match, try again");
			sleep(1);
			//TODO: improve this faster;
			find_connection(&entry,&(con_info.con_id));
		}
		debugf("Match, isn = %"PRIu32"", entry->isn);
		int iamleader = 0;
		ret = send(*sk, &iamleader, sizeof(iamleader), 0);
		if (ret < 0){
			printf("Failed to send reply back");
			pthread_exit(0);
		}

		con_info.isn = entry->isn;

		char* buffer;
		ret = con_info_serialize(&buffer, &len, &con_info);

		ret = send_bytes(*sk, buffer, len);

	    if (ret < 0)
	        perror("Failed to send con_info");

		close(*sk);
		pthread_exit(0);
	}	
}



int main(int argc, char *argv[]){
	init_con_hashmap();
	pthread_t report_thread;
	int ret;


	ret = pthread_create(&report_thread, NULL, serve_report, NULL);

	if (ret != 0){
		perror("Failed to create pthred");
		exit(1);
	} 


	//DEBUG
	con_list_entry *entry = (con_list_entry *)malloc(sizeof(con_list_entry));
	entry->con_id.src_port = htons(9999);
	entry->con_id.src_ip = 16777343;
	entry->con_id.dst_ip = 16777343;
	entry->con_id.dst_port = htons(10060);
	entry->isn = 931209;

	insert_connection(entry);




	int sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sk < 0){
		perror("Cannot create the socket");
		return -1;
	}


	struct sockaddr_in srvaddr;

	memset(&srvaddr, 0, sizeof(srvaddr));

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(QUERY_PORT); 

	debugf("binding to port: %d", QUERY_PORT);

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
		}
		pthread_t thread1;
		ret = pthread_create(&thread1, NULL, &serve, (void *)&ask);
		if (ret < 0){
			pthread_exit(0);
		}
	}
}


