#ifndef RSM_INTERFACE_H
#define RSM_INTERFACE_H
#include <unistd.h>
#include <stdint.h>

struct proxy_node_t;

#ifdef __cplusplus
extern "C" {
#endif
	int packet_buffer_to_buffer(uint8_t *buffer, int maxlen);
	int msg_handle(uint8_t *buf, int size);
	//struct proxy_node_t* proxy_init(const char* config_path, const char* proxy_log_path);
	//void proxy_on_read(struct proxy_node_t* proxy, void* buf, ssize_t ret, int fd);
	//void proxy_on_accept(struct proxy_node_t* proxy, int ret);
	//void proxy_on_close(struct proxy_node_t* proxy, int fildes);
	int is_leader(void);
	int read_from_packet_buffer(uint8_t **buffer);
#ifdef __cplusplus
}
#endif

#endif
