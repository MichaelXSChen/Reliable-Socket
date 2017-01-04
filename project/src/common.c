#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "include/common.h"

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


void perrorf(const char* format,...){
	int en = errno;
	va_list args;
	fprintf(stderr, "[DEBUG]: ");
	va_start (args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, " : %s\n", strerror(en));

}