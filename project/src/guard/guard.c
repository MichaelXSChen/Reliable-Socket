#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <errno.h>
//#include <signal.h>


#define DEBUG_ON 1 


void debug(const char* format,...){
	if (DEBUG_ON){
		va_list args;
		fprintf(stderr, "[DEBUG]: ");
		va_start (args, format);
		vfprintf(stderr, format, args);
		va_end(args);
		fprintf(stderr, "\n");
	}
}


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



int main(int argc, char *argv){
	pid_t pid, sid;
	

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

	//Main logic here
	// FILE *fp;
	// fp = fopen("Log.txt", "w+");
	// while(1){
	// 	sleep(1);
	// 	fprintf(fp, "logging info\n");
	// 	fflush(fp);
	// }
	// fclose(fp);



	int ret;
	char* argv_child[]={"/home/michael/VPB/test/tcp-how/tcp-howto", "127.0.0.1", "9999", NULL};
	ret = exec_task(&pid, argv_child);
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
	monitor();
	return 0;
}
