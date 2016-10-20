#ifndef RSM_INTERFACE_H
#define RSM_INTERFACE_H
#include <unistd.h>
#include <stdint.h>

#include "./output/output.h"

#ifdef __cplusplus
extern "C" {
#endif

	void mgr_init(uint8_t node_id,const char* config_path,const char* log_path);
	int msg_handle(uint8_t* msg,int size);
#ifdef __cplusplus
}
#endif

#endif
