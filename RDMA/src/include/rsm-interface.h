#ifndef RSM_INTERFACE_H
#define RSM_INTERFACE_H
#include <unistd.h>
#include <stdint.h>

#include "./output/output.h"

#ifdef __cplusplus
extern "C" {
#endif

	void mgr_init(uint8_t node_id,const char* config_path,const char* log_path);
#ifdef __cplusplus
}
#endif

#endif
