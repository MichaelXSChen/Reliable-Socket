#include <stdio.h>

int main(char **argv, int argc){
	int i = 0;
	while(1){
		printf("%d\n", i++);
		sleep(100);	
	}
}

