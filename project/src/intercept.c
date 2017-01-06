#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include "include/common.h"
#include "include/util.h"
 
#define CON_MGR_PORT 4321 
#define CON_MGR_IP "127.0.0.1"

typedef int (*orig_connect_func_type)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
typedef int (*orig_accpet_func_type)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*orig_socket_func_type)(int domain, int type, int protocol);
typedef int (*orig_bind_func_type)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);


typedef uint32_t u32;

//int sock; 


static int tcp_repair_on(int fd)
{
    int ret, aux = 1;

    ret = setsockopt(fd, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
    if (ret < 0)
        perror("Can't turn TCP repair mode ON");
     return ret;
}


static int tcp_repair_off(int fd)
{
    int ret, aux = 0;

    ret = setsockopt(fd, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
    if (ret < 0)
        perror("Can't turn TCP repair mode ON");
     return ret;
}

static int get_tcp_queue_seq(int sk, int queue, u32 *seq){
    debug("sk: %d", sk);
    if (setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue, sizeof(queue)) < 0){
        perror("CANNOT SET TCP Repair Queue");
        return -1;
    }
    if (getsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, seq, NULL) < 0){
        perror("Cannot GET tcp sequece number");
        return -1;
    }
}



static int set_tcp_queue_seq(int sk, int queue, u32 seq)
{

    if (setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue, sizeof(queue)) < 0) {
        perror("SET TCP QUEUE SEQ: cannot set TCP_REPAIR_QUEUE");
        return -1;
    }

    if (setsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, &seq, sizeof(seq)) < 0) {
        perror("SET TCP QUEUE SEQ: cannot set the tcp seq");
        return -1;
    }

    return 0;
}




/**

int set_sequence (int sk, u32 seq) {
    int aux = 1;
    int ret = 0; 
    ret = setsockopt(sk, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
        if (ret < 0) {
            perror("Cannot turn on TCP_REPAIR mode");
            return ret;
        }
    socklen_t auxl;
    int queue_id = TCP_SEND_QUEUE;
    auxl = sizeof(queue_id);
    ret = setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue_id, auxl);
    if (ret < 0) {
        perror("Failed to set to TCP_REPAIR_QUEUE for sentqueue");
        return ret; 
    }
    
    ret = setsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, &seq, auxl);
    if (ret < 0) {
        perror("Failed to set the TCP send queue seq");
        return ret;
    }
    printf("queue id: %" PRIu32 "\n", seq);

    aux = 0;
    ret = setsockopt(sk, SOL_TCP, TCP_REPAIR, &aux, sizeof(auxyao));
        if (ret < 0) {
            perror("Cannot turn on TCP_REPAIR mode");
            return ret;
        }

}
**/

int bind (int sockfd, const struct sockaddr *addr, socklen_t socklen){
    printf("Bind Function intercepted\n\n\n");
    fflush(stdout);

    int aux = 1;
    int ret;

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &aux, sizeof(aux));
    if (ret < 0){
        perror("failed to setup reuse");
        return -1;
    }

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &aux, sizeof(aux));
    if (ret < 0){
        perror("failed to setup reuse");
        return -1;
    }


    orig_bind_func_type orig_bind_func = (orig_bind_func_type) dlsym (RTLD_NEXT, "bind");
    return orig_bind_func(sockfd, addr, socklen);

}




int replace_tcp (int *sk, struct sockaddr_in bind_address, struct sockaddr_in connect_address){
    int ret, aux; 
    struct tcp_info ti; 
    struct sockaddr addr_local, addr_remote;
    printf("sk: %d\n", *sk);
    fflush(stdout);


    // turn on Repair mode
    ret = tcp_repair_on(*sk);
    if (ret != 0){
        return ret;
    }
    
    // if (dump_opt(*sk, SOL_TCP, TCP_INFO, &ti)){
    //     perror("Failed to obtain TCP_INFO");
    //     return -1;
    // }




    // char *in_buf, *out_buf;
    // u32 inq_seq, outq_seq;
    // u32 inq_len;
    // u32 outq_len, unsq_len;

    // // The information got from refresh_inet_sk in criu
    // unsigned int rqlen, wqlen, uwqlen;

    // int size; 
    // if (ioctl(*sk, SIOCOUTQ, &size) == -1) {
    //     perror("Unable to get size of snd queue");
    //     return -1;
    // }

    // wqlen = size;
    // if (ioctl(*sk, SIOCOUTQNSD, &size) == -1) {
    //     perror("Unable to get size of unsent data");
    //     return -1;
    // }
    // uwqlen = size;

    // if (ioctl(*sk, SIOCINQ, &size) == -1) {
    //     perror("Unable to get size of recv queue");
    //     return -1;
    // }
    // rqlen = size;


    // /*
    //  * Read queue
    //  */

    // inq_len = rqlen; 
    // ret = tcp_stream_get_queue(*sk, TCP_RECV_QUEUE, &inq_seq, inq_len, &in_buf);
    // if (ret < 0){
    //     perror("Failed to get the RECV QUEUE");
    //     return -1;
    // }

    // /*
    //  * Write queue
    //  */

    // outq_len = wqlen;
    // unsq_len = uwqlen;

    // ret = tcp_stream_get_queue(*sk, TCP_SEND_QUEUE, &outq_seq, outq_len, &out_buf);
    // if (ret < 0){
    //     perror("Failed to get the Send QUEUE");
    //     return -1;
    // }

    // //TODO: Window size;

    // //TODO: Options

    // //options
    // int has_nodelay = 0; 
    // int nodelay = 0;
    // int has_cork = 0; 
    // int cork = 0;



    // if (dump_opt(*sk, SOL_TCP, TCP_NODELAY, &aux)){
    //     printf("failed to get the TCP_NODELAY opt\n");
    //     return -1;
    // }

    // if (aux) {
    //     has_nodelay = 1;
    //     nodelay = 1;
    // }

    // if (dump_opt(*sk, SOL_TCP, TCP_CORK, &aux)){
    //     printf("faied to get the TCP_CORK opt\n");
    //     return -1;
    // }

    // if (aux) {
    //     has_cork = 1;
    //     cork = 1;
    // }



    //shutdown(*sk, 0);
    //int sk_new = dup(*sk);
    ret = close(*sk);
    
 
    //int sk_new=0; 

    *sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    //Repair mode on
    //*sk = sk_new;
    ret = tcp_repair_on(*sk);
    if (ret != 0){
       return ret;
    }

    // //Restore tcp sequence
    // ret = set_tcp_queue_seq(sk_new, TCP_RECV_QUEUE, inq_seq - inq_len);
    // if (ret != 0) {
    //     perror("Failed to set recv queue");
    //     return -1;
    // }

    ret = set_tcp_queue_seq(*sk, TCP_SEND_QUEUE, 931209);
    if (ret != 0) {
        perror("Failed to set send queue");
        return -1;
    }

    // //bind to the addr. 
    // //TODO: not done
    aux = 1;
    ret = setsockopt(*sk, SOL_SOCKET, SO_REUSEADDR, &aux, sizeof(aux));
    if (ret < 0){
        perror("failed to setup reuse");
        return -1;
    }


    ret = setsockopt(*sk, SOL_SOCKET, SO_REUSEPORT, &aux, sizeof(aux));
    if (ret < 0){
        perror("failed to setup reuse");
        return -1;
    }


    if (bind(*sk, (struct sockaddr *)&bind_address, sizeof(bind_address)) != 0){
        // getsockopt(*sk, SOL_TCP, TCP_REPAIR, &aux, NULL);

        // printf("TCP_REPAIR of new sk: %d\n\n", aux);
        // fflush(stdout);




        perror("Cannot bind socket here");
        fflush(stdout);
        return -1;
    }
    //connect
    if (connect(*sk, (struct sockaddr *)&connect_address, sizeof(connect_address)) != 0){
        perror("Cannot connect the inet socket back");
        return -1;
    }
    //Restore TCP_OPTs


    //Restore TCP_queue
    // u32 len;
    // len = inq_len;
    // if (len && send_tcp_queue(sk_new, TCP_RECV_QUEUE, len, in_buf)) 
    //     return -1;

    // len = outq_len - unsq_len; 
    // if (len && send_tcp_queue(sk_new, TCP_SEND_QUEUE, len, out_buf))
    //     return -1;
    // len = unsq_len;
    // tcp_repair_off(sk_new);
    // if (len && __send_tcp_queue(sk_new, TCP_SEND_QUEUE, len, out_buf))
    //     return -1;
    // if (tcp_repair_on(sk_new))
    //     return -1;

    // //todo:    restore Tcp window
    // //TODO: restore the TCP-buffersize

    // if (has_nodelay && nodelay) {
    //     aux = 1;
    //     if (restore_opt(sk_new, SOL_TCP, TCP_NODELAY, &aux)){
    //         perror("failed to set nodelay");
    //         return -1;
    //     }

    // }

    // if (has_cork && cork) {
    //     aux = 1;
    //     if (restore_opt(sk_new, SOL_TCP, TCP_CORK, &aux)){
    //         perror("failed to set cork");
    //         return -1;
    //     }
    // }

    tcp_repair_off(*sk);
 
    return 0;
}



// int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    
//     int ret; 
//     printf("Coonect Function intercepted!!\n\n\n");
//     orig_connect_func_type orig_connect;
//     orig_connect = (orig_connect_func_type) dlsym(RTLD_NEXT,"connect");
    
//     tcp_repair_on(sockfd);
//     set_tcp_queue_seq(sockfd, TCP_SEND_QUEUE, 931209);
//     tcp_repair_off(sockfd);
    
//     ret = orig_connect(sockfd, addr, addrlen);
//     // printf("sk after connect : %d\n", sk);
//     // fflush(stdout);


//     // struct sockaddr_in cliaddr, srvaddr;
//     // memset(&cliaddr, 0, sizeof(struct sockaddr_in));
//     // cliaddr.sin_family = AF_INET;
//     // cliaddr.sin_port = htons(10060);
//     // ret = inet_aton("127.0.0.1", &cliaddr.sin_addr);
//     // if (ret < 0) {
//     //     perror("Can't convert addr");
//     //     return -1;
//     // }

//     // int port = 9999; 
//     // memset(&srvaddr, 0, sizeof(srvaddr));
//     // srvaddr.sin_family = AF_INET;
//     // srvaddr.sin_port = htons(port);
//     // ret = inet_aton("127.0.0.1", &srvaddr.sin_addr);
//     // if (ret < 0) {
//     //     perror("Can't convert addr");
//     //     return -1;
//     // }


//     // ret = replace_tcp(&sk, cliaddr, srvaddr);
//     // if (ret != 0) {
//     //     return ret;
//     // }
//     return ret;
// }





int handle_con_info(const struct con_info_type *con_info){
    
    int ret; 
    //Send the packet
    //If it is the leader, it will be consensusd
    //If it is a follower, it will get the ISN to apply locally.
    char *buf;
    int len;
    ret = con_info_serialize(&buf, &len, con_info);
    
    printf("\n");
    for (ret = 0; ret < len; ret ++){
        printf("%d:   %d\n", ret, buf[ret]);
        fflush(stdout);
    }

    printf("\n");

    if (ret < 0){
        perror("Failed to serialize connection info");
        return -1;
    }
    int sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sk < 0) {
        perror("Can't create socket");
        return -1;
    }
    

    struct sockaddr_in addr;
    debug("Connecting to %s:%d\n", CON_MGR_IP, CON_MGR_PORT);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    ret = inet_aton(CON_MGR_IP, &addr.sin_addr);
    if (ret < 0) {
        perror("Can't convert addr");
        return -1;
    }
    addr.sin_port = htons(CON_MGR_PORT);

    ret = connect(sk, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("Can't connect");
        return -1;
    }

    ret = send_bytes(sk, buf, len);

    if (ret < 0){
        perror("Failed to send con_info");
        return -1;
    }

    return 0;

    //recv return value from con -manager

}



int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    int ret, port;
    printf("accept func intercepted \n\n");
    fflush(stdout);
    int aux; 
    socklen_t len;

    orig_accpet_func_type orig_accept_func; 
    orig_accept_func = (orig_accpet_func_type) dlsym(RTLD_NEXT, "accept");
    int sk; 
    sk = orig_accept_func(sockfd, addr, addrlen);

    uint32_t seq; 

    // sleep(5);
    // ret = tcp_repair_on(sk);
    // if (ret < 0){
    //     perrorf("Failed turn on");
    // }
    // ret = get_tcp_queue_seq(sk, TCP_SEND_QUEUE, &seq);
    // if (ret < 0){
    //     perrorf("Failed to get tcp_seq");
    //     return -1;
    // }

    // ret = tcp_repair_off(sk);
    // if (ret < 0){
    //     perrorf("Failed turn off");
    // }
    seq =99999;

    struct con_id_type con_id;
    con_id.src_ip = 12345;
    con_id.src_port = 1122;
    con_id.dst_ip = 67890;
    con_id.dst_port = 2211;
    struct con_info_type con_info;
    con_info.con_id = con_id;
    con_info.isn = 931209;

    ret = handle_con_info(&con_info);
    if (ret < 0){
        perrorf("Failed to send con_info");
        return -1;
    }
    return sk; 
}



int accept_bk(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
	int ret, port;
	printf("accept func intercepted \n\n");
    fflush(stdout);
    int aux; 
    socklen_t len;

    // ret = getsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &aux, &len);
    // if (ret < 0){
    //     perror("failed to setup reuse");
    //     return -1;
    // }
    // printf("SO_REUSEADDR of listen socket: %d\n\n", aux);
    // fflush(stdout);


	orig_accpet_func_type orig_accept_func; 
	orig_accept_func = (orig_accpet_func_type) dlsym(RTLD_NEXT, "accept");
	int sk; 
	sk = orig_accept_func(sockfd, addr, addrlen);
	//printf("sk: %d\n", sk);
	//sleep(100);

	struct sockaddr_in cliaddr, srvaddr;
	memset(&cliaddr, 0, sizeof(struct sockaddr_in));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(10060);
    ret = inet_aton("127.0.0.1", &cliaddr.sin_addr);
    if (ret < 0) {
        perror("Can't convert addr");
        return -1;
    }
    //FIXME: This is incorrect !!
    port = 9997; 
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(port);
    srvaddr.sin_addr.s_addr = INADDR_ANY;
    if (ret < 0) {
        perror("Can't convert addr :");
        return -1;
    }
    

    //tcp_repair_on(sockfd);

    ret = replace_tcp(&sk, srvaddr, cliaddr);

    if (ret < 0){
    	printf("Failed to replcae tcp\n" );
    	return ret; 
    }

    //tcp_repair_off(sockfd);





	return sk; 
}


// int socket(int domain, int type, int protocol){
//     printf("socket func called:");
//     orig_socket_func_type orig_socket;
//     orig_socket = (orig_socket_func_type) dlsym(RTLD_NEXT,"socket");
//     int sk =  orig_socket(domain, type, protocol);
    

//     printf("sk: %d\n", sk);
//     fflush(stdout);
//     return sk;

// }

int is_leader(){
    return 1;     
}


