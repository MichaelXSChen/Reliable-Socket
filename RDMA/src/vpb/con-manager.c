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
#include <sys/un.h>

#include <netinet/tcp.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <netinet/ip.h>




#define GUEST_IP "10.22.1.100"
#define REPORT_PORT 6666
#define HB_PORT 6667


#define MY_IP "10.22.1.2"





pthread_cond_t become_leader; 
pthread_mutex_t become_leader_lock;


// pthread_rwlock_t isn_lock;
// pthread_rwlock_t out_lock;


DECLARE_LOCK(isn_lock);
DECLARE_LOCK(out_lock);

static int sk;
// static int guest_out_sk;

static int report_sk;
// con_list_entry *con_list;

static int hb_sk;

static int guest_out_sk; 


typedef struct con_isn_entry con_isn_entry;

extern pthread_mutex_t tcp_outgoing_lock;


con_isn_entry *con_isn_list;

int get_isn(uint32_t *isn, struct con_id_type *con){
	con_isn_entry *entry;
	
	LOCK(isn_lock);
	HASH_FIND(hh, con_isn_list, con, sizeof(struct con_id_type), entry);
	UNLOCK(isn_lock);
	

	if (entry == NULL){
		return -1;
	}
	*isn = entry->isn; 
	return 0;
}

int save_isn(uint32_t isn, struct con_id_type *con){
	con_isn_entry *entry = (con_isn_entry *) malloc(sizeof(con_isn_entry));
	memcpy(&(entry->con_id), con, sizeof(struct con_id_type));
	entry->isn = isn; 
	struct con_isn_entry *replaced;

 
	flush_con_out_seq(con);

	debugf("trying to insert into hashtable");
	
	LOCK(isn_lock);
	HASH_REPLACE(hh, con_isn_list, con_id, sizeof(struct con_id_type), entry, replaced);
	UNLOCK(isn_lock);



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
			pthread_exit(0);
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
		debugf("RECV_SEQ: %" PRIu32 "", con_info.recv_seq);
		debugf("has timestamp? %"PRIu16 " val = %"PRIu32"", con_info.has_timestamp, con_info.timestamp);
		debugf("MSS_CLAMP:%" PRIu32 "", con_info.mss_clamp);
		debugf("snd_wscale:%" PRIu32 "", con_info.snd_wscale);
		debugf("rcv_wscale:%" PRIu32 "", con_info.rcv_wscale);
		debugf("has rcv wsacle? %"PRIu16 "", con_info.has_rcv_wscale);



		
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

	//Wait for the initilization to be completed.
	sleep(5);

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



struct con_out_seq_entry{
	struct con_id_type con_id;
	uint32_t seq; 
	UT_hash_handle hh;
};

typedef struct con_out_seq_entry con_out_seq_entry; 

con_out_seq_entry *con_out_seq_list;



int flush_con_out_seq(struct con_id_type *server_con){





	con_out_seq_entry *entry = (con_out_seq_entry *) malloc(sizeof(con_out_seq_entry));
	memset(entry, 0, sizeof(con_out_seq_entry));

	entry->con_id.src_port = server_con->dst_port;
	entry->con_id.dst_port = server_con->src_port;

	entry->con_id.src_ip = server_con->dst_ip;
	entry->con_id.dst_ip = server_con->src_ip;

	entry->seq = 0;

	struct con_out_seq_entry *replaced;


	LOCK(out_lock);
	HASH_REPLACE(hh, con_out_seq_list, con_id, sizeof(struct con_id_type), entry, replaced);
	UNLOCK(out_lock);

	if (replaced != NULL)
		//debugf("Flushed outgoing seq, from %"PRIu32"to 0", replaced->seq);

		print_con(server_con, "Flush, Seq got replaced to 0 from "PRIu32"", replaced->seq);
	else
		//debugf("new connection form port %d to port %d, seq set to 0", ntohs(server_con->dst_port), ntohs(server_con->src_port));
		print_con(server_con, "New Connection, Set SEQ to 0\n\n\n\n\n");
	return 0;
}



int get_con_out_seq(uint32_t *seq, struct con_id_type *con_orig){
	con_out_seq_entry *entry;
	// struct in_addr src_ip, dst_ip;
	// src_ip.s_addr = con->src_ip;
	// dst_ip.s_addr = con->dst_ip;
	struct con_id_type *con = (struct con_id_type *) malloc(sizeof(struct con_id_type));
	memset(con, 0, sizeof(struct con_id_type));

	con->src_ip = con_orig->src_ip;
	con->dst_ip = con_orig->dst_ip;
	con->src_port = con_orig->src_port;
	con->dst_port = con_orig->dst_port;




	// debugf("[FIND]: Trying to find src_ip %s, src_port%"PRIu16" to dst_ip %s dst_port%"PRIu16"", inet_ntoa(src_ip), ntohs(con->src_port), inet_ntoa(dst_ip), ntohs(con->dst_port));

#ifndef MUTEX_LOCK
	LOCK(out_lock);
	HASH_FIND(hh, con_out_seq_list, con, sizeof(struct con_id_type), entry);
	UNLOCK(out_lock);
#else
	HASH_FIND(hh, con_out_seq_list, con, sizeof(struct con_id_type), entry);
#endif

	if (entry != NULL){
		*seq = entry->seq;
		return 0;
	}
	else{
		*seq = 0;
		//FIXIT: for debug usage, 1 is different from 0;
		print_con(con, "NO MATCH found\n\n\n\n");
		LOCK(out_lock);

		struct con_out_seq_entry *e;
		for (e = con_out_seq_list; e != NULL; e= e->hh.next){
			print_con(&(e->con_id), "seq =%" PRIu32"\n", e->seq); 
		}


		UNLOCK(out_lock);


		return -1;
	}
}

int update_con_out_seq(uint32_t seq, struct con_id_type *con_orig){
	
	struct con_id_type *con = (struct con_id_type *) malloc(sizeof(struct con_id_type));
	memset(con, 0, sizeof(struct con_id_type));

	con->src_ip = con_orig->src_ip;
	con->dst_ip = con_orig->dst_ip;
	con->src_port = con_orig->src_port;
	con->dst_port = con_orig->dst_port;




	con_out_seq_entry *find_entry;
	LOCK(out_lock);
	HASH_FIND(hh, con_out_seq_list, con, sizeof(struct con_id_type), find_entry);
	UNLOCK(out_lock);

	if (find_entry != NULL && find_entry->seq >= seq){
		//No need to update, only keep max value
		return 0;
	}

	con_out_seq_entry *entry = (con_out_seq_entry *) malloc(sizeof(con_out_seq_entry));
	memset(entry, 0, sizeof(con_out_seq_entry));


	memcpy(&(entry->con_id), con, sizeof(struct con_id_type));
	entry->seq = seq; 
	struct con_out_seq_entry *replaced;
	


	pthread_mutex_lock(&tcp_outgoing_lock);

	LOCK(out_lock);
	HASH_REPLACE(hh, con_out_seq_list, con_id, sizeof(struct con_id_type), entry, replaced);
	UNLOCK(out_lock);

	// struct in_addr src_ip, dst_ip;
	// src_ip.s_addr = con->src_ip;
	// dst_ip.s_addr = con->dst_ip;



	//debugf("[INSERT]: src_ip %s, src_port%"PRIu16" to dst_ip %s dst_port%"PRIu16", seq increased to %"PRIu32"", inet_ntoa(src_ip), ntohs(con->src_port), inet_ntoa(dst_ip), ntohs(con->dst_port), seq);
	wakeup_tcp();
	pthread_mutex_unlock(&tcp_outgoing_lock);

	return 0;
}

int check_block(uint32_t ack, struct con_id_type *con){
	//Return 0 if no need to block
	//Return 1 if need to block the incoming queue for a while
	//		because the outgoing packet hasn't been generated



	uint32_t isn; 
	int ret; 

	struct con_id_type con_id; 
	con_id.src_ip = con->dst_ip;
	con_id.src_port = con->dst_port;
	con_id.dst_ip = con->src_ip;
	con_id.dst_port = con->src_port;

	ret = get_isn(&isn, &con_id);
	if (ret < 0){
		debugf("not my managed connection");
		//This is not the packet manageed.  
		return 0;
	}
	
	uint32_t out_seq;
	ret = get_con_out_seq(&out_seq, con);
	if(out_seq  + 1 < ack){
		struct in_addr src_addr, dst_addr;
		src_addr.s_addr = con->src_ip;
		dst_addr.s_addr = con->dst_ip;
		debugf("out_seq: %"PRIu32" ack: %"PRIu32" block, (from %s:%d to %s:%d)", out_seq, ack, inet_ntoa(src_addr), ntohs(con->src_port), inet_ntoa(dst_addr), ntohs(con->dst_port));
		return 1;
	}
	else{
		debugf("out_seq: %"PRIu32" ack: %"PRIu32" PASS", out_seq, ack);

		return 0;
	}


}

#define MSG_OFF 10 //From ning: "the whole message offset, TODO:need to determine the source"

void *watch_guest_out(void *useless){
	char buf[2048];
	int len;
	int eth_hdr_len = sizeof(struct ether_header);
	while(1){
		len = recv(guest_out_sk, buf, sizeof(buf), 0);
		//analyze the outgoing packets
		//debugf("recv len: %d", len);
		if(len > eth_hdr_len + MSG_OFF){
			struct ether_header* eth_hdr = (struct ether_header*)((uint8_t*)buf + MSG_OFF);
			// if (eth_hdr->ether_type == ETHERTYPE_ARP){
			// 	debugf("ARP PACKET");
			// 	continue;
			// }
			//debugf("ether_type %04x", eth_hdr->ether_type);
			if (eth_hdr->ether_type == 0x0008){
				//debugf("[OUTGOING] IP Packet intercepted");


				struct ip* ip_header = (struct ip*)((uint8_t*)buf + eth_hdr_len + MSG_OFF);
				if (ip_header->ip_p == 0x06){
					//TCP PACKET
					//debugf("[OUTGOING] TCP Packet intercepted");




					int  ip_header_size = 4 * (ip_header->ip_hl & 0x0F); //Get the length of ip_header;
	            	short ip_len = ntohs(ip_header->ip_len); 
	            	struct tcphdr* tcp_header = (struct tcphdr*)((uint8_t*)buf + eth_hdr_len + ip_header_size + MSG_OFF);

	            	/****************************
	                *That is outgoing packet, in roder to match src and dst for incoming packet, 
	                *Save it in
	                *!!Opposite!!
	                *******************************/
	            	struct con_id_type con_id; 
	                con_id.src_ip = ip_header->ip_dst.s_addr;
	                con_id.src_port = tcp_header->th_dport;
	                con_id.dst_ip = ip_header->ip_src.s_addr;
	                con_id.dst_port = tcp_header->th_sport;

	                //Don't forget ntohl to compare

	                int tcp_header_size = 4 * (tcp_header->th_off & 0X0F);
	                int payload_length = ip_len - ip_header_size - tcp_header_size; 
	                if (payload_length < 0)
	                	payload_length = 0;


	                // printf("outgoing *****************\n");
	                // printf("len = %d\n", len);
	                // printf("eth_hdr_len = %d\n",eth_hdr_len);
	                // printf("ip_len = %d\n", ip_len);
	                // printf("ip_header_len = %d\n", ip_header_size);
	                // printf("tcp_header_len = %d\n", tcp_header_size);
	                // printf("payload_length = %d\n", payload_length);

	                uint32_t seq = ntohl(tcp_header->th_seq) + payload_length;

	                print_con(&con_id, "pakcet len: %d, seq = %"PRIu32" + %d = %"PRIu32"", len,ntohl(tcp_header->th_seq), payload_length, seq);


	                // printf("seq increased to %"PRIu32"\n", seq);
	                // //int i = 0;
	                // // for (i = 0; i<len; i++){
	                // // 	printf("%#02x  ", buf[i]&0xff);
	                // 	if ((i+1) % 8 == 0){
	                // 		printf("     ");
	                // 		if((i+1) %16 == 0){
	                // 			printf("\n");
	                // 		}
	                // 	}
	                // }
	                // printf("\n");



	                // printf("**************************\n\n");
	                // fflush(stdout);

	                update_con_out_seq(seq, &con_id);
	            }
			}
		}

	}
}

int con_manager_init(){
	
	con_isn_list = NULL;
	con_out_seq_list = NULL;

	debugf("\n\n\n\n\n\ncon_manager_init INIT CALLED !!!\n\n\n\n\n\n\n\n");


	pthread_cond_init(&become_leader, NULL);
    pthread_mutex_init(&become_leader_lock, NULL);


    LOCK_INIT(isn_lock);
    LOCK_INIT(out_lock);

    char *server_filename = "/tmp/socket-server";

    guest_out_sk = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (guest_out_sk < 0){
        perror("Failed to create unix socket for listening");
        exit(1);
    }

    unlink(server_filename);

    struct sockaddr_un unix_addr; 
    memset(&unix_addr, 9, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strncpy(unix_addr.sun_path, server_filename, 104);

    int ret_val;

    ret_val = bind(guest_out_sk, (struct sockaddr*)&unix_addr, sizeof(unix_addr));
    if (ret_val == -1){
        perror("Failed to bind listeing unix socket");
        exit(1);
    }

    pthread_t watch_output_thread;
    pthread_create(&watch_output_thread, NULL, watch_guest_out, NULL);





	// Init the hash table

	
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

	ret = listen(sk, SOMAXCONN);
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


