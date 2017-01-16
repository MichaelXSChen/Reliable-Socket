#include <pthread.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <semaphore.h>

#include "include/common.h"
#include "include/con-manager-guest.h"

int exec_task(pid_t *pid, struct command_t *cmd);
int monitor();

#define COMMAND_PORT 3120


#define DEBUG_NO_DAEMON
#define DEBUG_NO_SHUTDOWN


static int sk_listen_command_con;

static sem_t have_child;


int exec_task(pid_t *pid, struct command_t *cmd){
	int ret;

	//TODO: safety check, the last of argv must be NULL
	if(cmd->argv[cmd->argc-1]){
		//if it is not null
		struct command_t *cmd_new = (struct command_t *) malloc(sizeof(struct command_t));
		cmd_new->argc = cmd->argc +1;
		cmd_new->argv = (char **)malloc( (cmd->argc +1) * sizeof(char*));
		int i;
		for (i = 0; i< cmd->argc; i++){
			cmd_new->argv[i]=cmd->argv[i];
		}
		cmd_new ->argv[cmd->argc] = NULL;
		free(cmd);
		cmd = cmd_new;
	}

	int i; 
	for (i = 0; i<cmd->argc; i++){
		debugf("Command argv[%d]: %s", i, cmd->argv[i]);
	}

    pid_t child_pid = 0;
    child_pid = fork();
    if (child_pid < 0){
    	perror("Error forking");
    	return -1;
    }

    sem_post(&have_child);

    if (child_pid == 0) {
    	ret = execv(cmd->argv[0], cmd->argv);
		if (ret != 0){
			fprintf(stderr, "Failed to run execv %s\n", strerror(errno));
			exit(0);
		}
	}
	*pid = child_pid;
	free(cmd);
    return 0;
}

int monitor() {
	
	while(1){
		sem_wait(&have_child);
		// int rev; 
		// sem_getvalue(&have_child, &rev);
		// debugf("Number of child: %d", rev);


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
			debugf("Process %d exit with exit value: %d", (int)pid, exstat);
		}
		else if (WIFSIGNALED(status)){
			int sig = WTERMSIG(status);
			//TODO: what to do here??
			debugf("Process %d exit because of signal: %s", (int)pid, strsignal(sig));
		}
	}
}





void *serve_command_connection(void* arg){
	int sk = *(int*)arg;
	free(arg);
	while(1){
		char *buf;
		int length, ret;
		ret = recv_bytes(sk, &buf, &length);
		if (ret < 0){
			perrorf("Failed to recv");
			pthread_exit(0);
		}
		if(ret == 0){
			//The connection is closed
			debugf("Connection for Recving commands Closed");
			pthread_exit(0);
		}
		else{
			debugf("Received Command with length %d", length);
			struct command_t *cmd = (struct command_t*)malloc(sizeof(struct command_t)); 

			command_t_deserialize(cmd, buf, length);
			pid_t pid; 
			exec_task(&pid, cmd);
			debugf("Forked a child process (pid = %d) to execute command", pid);

		}
	}


	pthread_exit(0);
}


void* guard_listen(void *arg) {

	//debugf("Wating for connections");


	int ask, pid; 

	while(1){
		ask = accept(sk_listen_command_con, NULL, NULL);
		if (ask < 0){
			perror("Cannot accept new conn");
			pthread_exit(0);
		}
		int *arg = (int*)malloc(sizeof(int));
		*arg = ask;
		pthread_t thread;
		pthread_create(&thread, NULL, serve_command_connection, (void*)arg);

	}

	
}




int main(int argc, char* argv[]){
	init_con_manager_guest();
	
	sem_init(&have_child, 0, 0);

	pid_t pid;

#ifndef DEBUG_NO_DAEMON
	pid = fork();
	if (pid < 0){
		perrorf("Error forking\n");
		return 1;
	}
	if (pid > 0){
		// Let the 
		debugf("PID of child proces %d\n", pid);
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
#endif



	int ret;


	struct sockaddr_in addr;

	sk_listen_command_con = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sk_listen_command_con < 0){
		perror("Cannot create socket");
		pthread_exit(0);
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(COMMAND_PORT);

	debugf("[SK-%d]: Listening for Command on port %d", sk_listen_command_con, COMMAND_PORT);
	ret = bind(sk_listen_command_con, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0){
		perror("[ERROR]: Cannot bind to socket");
		pthread_exit(0);
	}
	ret = listen(sk_listen_command_con, 16);
	if (ret < 0){
		perror("Cannot put socket to listen state");
		pthread_exit(0);
	}




	pthread_t listen_thread; 
	pthread_create(&listen_thread, NULL, guard_listen, NULL);
	debugf("Guest Daemon Initialization Completed");

	monitor();
}


