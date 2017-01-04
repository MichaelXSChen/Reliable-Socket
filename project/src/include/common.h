#include <stdint.h>
#ifndef COMMON_H
#define COOMON_H

#define DEBUG_ON 1 

void debug(const char* format,...);


void perrorf(const char* format,...);


struct arg_command{
	char **argv;
	int32_t argc;
};

int arg_command_serialize(char **buf, int *len, const struct arg_command* cmd);

int arg_command_deserialize(struct arg_command *cmd, const char *buf, int len);
 
#endif // COMMON_H