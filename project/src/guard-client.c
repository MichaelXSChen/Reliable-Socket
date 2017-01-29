#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>



#include "include/common.h"



int guard_client(char* ip, int port){
	

}


int main(int argc, char *argv[]){
	char *ip = "10.22.1.100";
    int port = 3120;




    int sk, ret;
    struct sockaddr_in addr;

    sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sk < 0) {
        perror("Can't create socket");
        return -1;
    }

    debugf("Connecting to %s:%d\n", ip, port);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    ret = inet_aton(ip, &addr.sin_addr);
    if (ret < 0) {
        perror("Can't convert addr");
        return -1;
    }
    addr.sin_port = htons(port);

    ret = connect(sk, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("Can't connect");
        return -1;
    }
    debugf("connected");
    


    struct command_t cmd; 


    char** command = (char**) malloc((argc-1)*sizeof(char*));

    int i;
    for(i = 0; i< argc-1; i ++){
        command[i]=argv[i+1];
    } 


    cmd.argv = command;
    cmd.argc = argc-1;

    char *buf; 
    int len; 
    command_t_serialize(&buf, &len, &cmd);

    ret = send_bytes(sk, buf, len);
    if (ret < 0){
        perrorf("Failed to send");
        return -1;
    }
    sleep(5);

    close(sk);
}
