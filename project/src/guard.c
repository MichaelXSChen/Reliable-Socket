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
#include "include/tpl.h"
#include "include/util.h"
//#include <signal.h>




// int main_loop_test(){
// 	// if (chdir("") == -1){
// 	// 	printf("Failed to chdir");
// 	// 	return -1;
// 	// }
	
// 	int ret = 0;
// 	ret = execv(argv[0], argv);
// 	if (ret != 0){
// 		perror("execv failed");
// 	}
// 	printf("ret: %d\n", ret);
// 	fflush(stdout);
// 	return ret;
// }



int exec_task(pid_t *pid, char* argv[]){
	int ret;

	//TODO: safety check, the last of argv must be NULL

    pid_t child_pid = 0;
    child_pid = fork();
    if (child_pid < 0){
    	perror("Error forking");
    	return -1;
    }

    if (child_pid == 0) {
    	ret = execv(argv[0], argv);
		if (ret != 0){
			fprintf(stderr, "Failed to run execv %s\n", strerror(errno));
			return -1;
		}
	}
	*pid = child_pid;
    return 0;
}

int monitor() {
	int status;
	pid_t pid; 
	pid = waitpid(0, &status, 0);
	if (pid < 0){
		perror("Failed while executing waitpid");
		return -1;
	}
	if (WIFEXITED(status)){
		//Exited by return or exit
		int exstat = WEXITSTATUS(status);
		//TODO: what to do here??
		debug("Process %d exit with exit value: %d", (int)pid, exstat);
	}
	else if (WIFSIGNALED(status)){
		int sig = WTERMSIG(status);
		//TODO: what to do here??
		debug("Process %d exit because of signal: %s", (int)pid, strsignal(sig));
	}
}




int serve_conn(int sk){
	while(1){
		char *buf;
		int length, ret;
		ret = recv_bytes(sk, &buf, &length);
		if (ret < 0){
			perrorf("Failed to recv");
			return -1;
		}
		else{
			debug("length %d", length);

			//debug("strlen: %d", strlen(buf));
			debug("str: %s", buf);
		}
	}


	return 0;
}

int guard_listen(int port) {
	int sk,ret;


	struct sockaddr_in addr;

	sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sk < 0){
		perror("Cannot create socket");
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	debug("Bingd to port %d", port);
	ret = bind(sk, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0){
		perror("[ERROR]: Cannot bind to socket");
		return -1;
	}
	ret = listen(sk, 16);
	if (ret < 0){
		perror("Cannot put socket to listen state");
		return ret;
	}
	debug("Wating for connections");


	int ask, pid; 


	ask = accept(sk, NULL, NULL);
	if (ask < 0){
		perror("Cannot accept new conn");
		return -1;
	}
	close(sk);

	serve_conn(ask);
	return 0;

}



// struct command_type{
// 	int argc;
// 	char** argv;
// };


// int deserialize(char**, char *str, int size,  ){
// 	char** out;
// 	tl_node *tn;
// 	tn = tpl_map("s#");
// }



int main(int argc, char *argv){
	pid_t pid, sid;
	
	
	char* array[]={"/home/michael/VPB/test/tcp-how/tcp-howto", "127.0.0.1", "9999", NULL};
	struct arg_command cmd, cmd2;
	cmd.argc = 4;
	cmd.argv = array;
	//debug("%s", cmd.argv[2]);
	char* buf;
	int len;
	arg_command_serialize(&buf,&len, &cmd);
	debug("Len: %d", len);
	
	arg_command_deserialize(&cmd2, buf, len);
	//This make the process a deamon program/
	/*
	pid = fork();
	if (pid < 0){
		printf("Error forking\n");
		return 1;
	}
	if (pid > 0){
		printf("PID of child proces %d\n", pid);
		return 0;
	}
	
	umask(0);
	sid = setsid();
	if (sid < 0){
		printf("Error Setting session id\n");
		return 1;
	}
	chdir("/");
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	*/




	int ret;
	char* argv_child[]={"/home/michael/VPB/test/tcp-how/tcp-howto", "127.0.0.1", "9999", NULL};
	//ret = exec_task(&pid, argv_child);
	// if (pid == 0){
	// 	ret = main_loop_test();
	// }
	// else{
	// int status;
	// ret = waitpid(pid, &status, 0);
	// if (ret == -1){
	// 	//TODO: understand why return -1;
	// 	perror("faild to wait pid");
	// }
	// else {
	// 	printf("Child process end with exit value (%d)\n\n", status);
	// }
	// }

	//guard_listen(12346);	
	//monitor();
	return 0;
}


