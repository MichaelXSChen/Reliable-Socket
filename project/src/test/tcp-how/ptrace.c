#include <sys/ptrace.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

int main(int argc, char **argv){
	if (argc != 2){
		printf("Usage: ./ptrace pid");
	}
	pid_t target_pid = (pid_t) atol(argv[1]);
	long ret; 
	ret = ptrace(PTRACE_ATTACH, target_pid, NULL, NULL);
	if (ret != 0){
		perror ("attach to process");
		return -1;
	}

	int status; 
	waitpid(target_pid, &status, WUNTRACED);

	printf("wait passes");
	sleep(20);
	return 0;

}