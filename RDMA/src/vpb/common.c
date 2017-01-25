#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#include "../include/vpb/tpl.h"
#include "../include/vpb/common.h"



void debugf(const char* format,...){
	if (DEBUG_ON){
		va_list args;
		fprintf(stderr, "[DEBUG]: ");
		va_start (args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		fprintf(stderr, "\n");
	}
}


void perrorf(const char* format,...){
	int en = errno;
	va_list args;
	fprintf(stderr, "[DEBUG]: ");
	va_start (args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, " : %s\n", strerror(en));

}

int command_t_serialize(char **buf, int *len, const struct command_t* cmd){
	tpl_node *tn;
	//debugf("size %d", cmd->argc);
	//return 0;

	tn = tpl_map("s#", cmd->argv, cmd->argc);
	tpl_pack(tn, 0);

	int size;
	char* buffer; 
	tpl_dump(tn, TPL_MEM, &buffer, &size);
	debugf("size%d", size);
	tpl_free(tn);
	*len = sizeof(cmd->argc)+size;

	(*buf) = (char *)malloc(*len);
	//debugf("len: %d", *len);
	memcpy((*buf), &cmd->argc, sizeof(cmd->argc));
	//memset(&((*buf)[sizeof(cmd->argc)]), 1, *len);
	memcpy(&((*buf)[sizeof(cmd->argc)]), buffer, size);
	free(buffer);
	// int i = 0;
	// for (i = 0; i<*len; i++){
	// 	printf("%d\n",(*buf)[i]);
	// 	fflush(stdout);
	// }
	return 0;

}


int command_t_deserialize(struct command_t *cmd, const char *buf, int len){
	int32_t argc=0;

	memcpy(&argc, buf, sizeof(argc));
	tpl_node *tn;
	char * argv[argc];
	tn = tpl_map("s#", &argv, argc);

	tpl_load(tn, TPL_MEM, &buf[sizeof(argc)], len-sizeof(argc));
	tpl_unpack(tn,0);
	tpl_free(tn);
	int i;
	fflush(stdout);
	cmd->argc = argc;
	cmd->argv = (char**) malloc(argc * sizeof(char*));
	for (i = 0; i<argc; i++){
		cmd->argv[i]= argv[i];
	}



	return 0;
}

int con_info_serialize(char **buf, int *len, const struct con_info_type *con_info){
	tpl_node *tn;
	tn = tpl_map("S($(uvuv)uuvuuuuv)", con_info);
	tpl_pack(tn, 0);
	tpl_dump(tn, TPL_MEM, buf, len);
	tpl_free(tn);
	return 0;
}



int con_info_deserialize(struct con_info_type *con_info, const char *buf, int len){
	//debugf("deserialize called, len: %d", len);
	tpl_node *tn;
	tn = tpl_map("S($(uvuv)uuvuuuuv)", con_info);
	tpl_load(tn, TPL_MEM, buf, len);
	tpl_unpack(tn, 0);
	tpl_free(tn);
	return 0;
}




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
		if (ret == 0)
			return 0;
		else{
			data += ret;
			left -= ret;
		}
	}while(left >0);



	debugf("recv bytes with length: %d", *length);
	return *length;
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

	debugf("Sent bytes with len: %d", len);
	return 0;
}


void print_con (struct con_id_type *con_id, const char* format,...){
	struct in_addr src_addr;
	struct in_addr dst_addr;
	
	src_addr.s_addr = con_id->src_ip;
	dst_addr.s_addr = con_id->dst_ip;



	va_list args;
	fprintf(stderr,"[CON] %s:%d ", inet_ntoa(src_addr), ntohs(con_id->src_port));
	frpintf("to %s:%d ",  inet_ntoa(dst_addr), ntohs(con_id->dst_port));
	va_start (args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");


}