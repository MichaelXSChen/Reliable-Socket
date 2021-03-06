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
#include <stdlib.h>

#include "include/common.h"
 
#define CON_MGR_PORT 7777 
#define CON_MGR_IP "127.0.0.1"
#define MY_IP "10.22.1.100"
#define QEMU_PORT 4321







typedef int (*orig_connect_func_type)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
typedef int (*orig_accept_func_type)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*orig_socket_func_type)(int domain, int type, int protocol);
typedef int (*orig_bind_func_type)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
typedef int (*orig_accept4_func_type)(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);




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

static int get_tcp_queue_seq(int sk, int queue, uint32_t *seq){
    if (setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue, sizeof(queue)) < 0){
        perror("CANNOT SET TCP Repair Queue");
        return -1;
    }
    socklen_t len = sizeof(*seq);
    if (getsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, seq, &len) < 0){
        perror("Cannot GET tcp sequece number");
        return -1;
    }
    

}



static int set_tcp_queue_seq(int sk, int queue, uint32_t seq)
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

static int get_tcp_con_id(int sk, struct con_info_type *con_info){
    struct sockaddr_in localaddr, remoteaddr;
    int len = sizeof(localaddr);
    int ret; 

    ret = getpeername(sk, (struct sockaddr*)&remoteaddr, &len);
    if (ret != 0){
        perrorf("Failed to get addr for sk: %d", sk);
        return -1;
    }
    con_info->con_id.dst_ip = remoteaddr.sin_addr.s_addr;
    con_info->con_id.dst_port = remoteaddr.sin_port;


    ret = getsockname(sk, (struct sockaddr*)&localaddr, &len);
    if (ret != 0){
        perrorf("Failed to get addr for sk: %d", sk);
        return -1;
    }
    con_info->con_id.src_ip = localaddr.sin_addr.s_addr;
    con_info->con_id.src_port = localaddr.sin_port;


    return 0;

}



int bind (int sockfd, const struct sockaddr *addr, socklen_t socklen){
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




int replace_tcp (int *sk, struct con_info_type *con_info){
    orig_connect_func_type orig_connect_func; 
    orig_connect_func = (orig_connect_func_type) dlsym(RTLD_NEXT, "connect");    


    int ret, aux; 
    struct sockaddr_in localaddr, remoteaddr;
    int len=sizeof(localaddr);
    
    //Turn on Repair mode, getsockname and close the old tcp
    ret = tcp_repair_on(*sk);
    if (ret != 0){
        perrorf("1:%d", *sk);
        return ret;
    }
    

    ret = getsockname(*sk, (struct sockaddr*)&localaddr, &len);
    if (ret != 0){
        perrorf("Failed to get addr for sk: %d", sk);
        return -1;
    }

    memset(&remoteaddr, 0, sizeof(remoteaddr));
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_port = con_info->con_id.dst_port;
    remoteaddr.sin_addr.s_addr = con_info->con_id.dst_ip; 


    ret = close(*sk);
    
 

    // Create a new socket, connect to the original address. 
    *sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    ret = tcp_repair_on(*sk);
    if (ret != 0){
        perror("2");
        return ret;
    }
    ret = set_tcp_queue_seq(*sk, TCP_RECV_QUEUE, con_info->recv_seq);
    if (ret != 0) {
        perror("Failed to set recv queue seq");
        return -1;
    }

    ret = set_tcp_queue_seq(*sk, TCP_SEND_QUEUE, con_info->send_seq-1);
    if (ret != 0) {
        perror("Failed to set send queue seq");
        return -1;
    }



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


    if (bind(*sk, (struct sockaddr *)&localaddr, sizeof(localaddr)) != 0){
        perror("Cannot bind socket here");
        return -1;
    }

    if (orig_connect_func(*sk, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr)) != 0){
        perrorf("Cannot connect the socket to %s:%d", inet_ntoa(remoteaddr.sin_addr), ntohs(remoteaddr.sin_port));
        return -1;
    }

    debugf("[LD_PRELOAD] send_seq: %"PRIu32"recv_seq: %"PRIu32"", con_info->send_seq, con_info->recv_seq);

    if (con_info->has_timestamp){
        debugf("[LD_PRELOAD] Will turn TCP TIMESTAMP on, timestamp = %"PRIu32"", con_info->timestamp);
        struct tcp_repair_opt opt;
        opt.opt_code = TCPOPT_TIMESTAMP;
        opt.opt_val = 0;
        ret = setsockopt(*sk, SOL_TCP, TCP_REPAIR_OPTIONS, &opt, sizeof(struct tcp_repair_opt));
        if (ret < 0){
            perrorf("Failed to repair TCP OPTIONS");
            return -1;
        }
        ret = setsockopt(*sk, SOL_TCP, TCP_TIMESTAMP, &(con_info->timestamp), sizeof(con_info->timestamp));
        if (ret < 0){
            perrorf("Failed to set tcp timestamp");
            return -1;
        }
    }

    struct tcp_repair_opt opt_for_mss;  
    opt_for_mss.opt_code = TCPOPT_MAXSEG;
    opt_for_mss.opt_val = con_info->mss_clamp;

    ret = setsockopt(*sk, SOL_TCP, TCP_REPAIR_OPTIONS, &opt_for_mss, sizeof(struct tcp_repair_opt));
    if (ret < 0){
        perrorf("Failed to set mss_clamp");
        return -1;
    }
    
    debugf("mss_size: %"PRIu32"", con_info->mss_clamp);




    if (con_info->has_rcv_wscale){
        struct tcp_repair_opt opt_for_wscale;
        opt_for_wscale.opt_code = TCPOPT_WINDOW;

        opt_for_wscale.opt_val = con_info->snd_wscale + (con_info->rcv_wscale << 16);
        ret = setsockopt(*sk, SOL_TCP, TCP_REPAIR_OPTIONS, &opt_for_wscale, sizeof(struct tcp_repair_opt));
        if (ret < 0){
            perrorf("Faield to set wscale");
            return -1;
        } 
        debugf("snd_wscale: %"PRIu32", recv_wscale: %"PRIu32"", con_info->snd_wscale, con_info->rcv_wscale);

    }


    debugf("(Backup) created socket to %s:%"PRIu16"", inet_ntoa(remoteaddr.sin_addr), ntohs(remoteaddr.sin_port));
    
    uint8_t triger = 1;
    ret = send(*sk, &triger, 1, 0);
    debugf("[Intercept.so]  Sent out 1 bytes for trigering");

    tcp_repair_off(*sk);
 
    return 0;
}



int ask_for_consensus(int sk_tomgr, struct con_info_type *con_info){
    char *buf;
    int ret,len;
    ret = con_info_serialize(&buf, &len, con_info);
    if (ret < 0){
        perror("Failed to serialize connection info");
        return -1;
    }
    
    ret = send_bytes(sk_tomgr, buf, len);
    if (ret < 0){
        perror("Failed to send con_info");
        return -1;
    }
    debugf("Sent for asking for consensus , waiting for result");


    uint8_t success; 
    ret = recv(sk_tomgr, &success, sizeof(success), 0);
    debugf("Successfully asked for consensus");
    free(buf);
    return 0;
}


int handle_accepted_sk(int *sk){
    
    
    orig_connect_func_type orig_connect_func; 
    orig_connect_func = (orig_connect_func_type) dlsym(RTLD_NEXT, "connect");


    int ret; 
    /*********
    /Check whether it is leader
    **************/

    //Connect to manager
    int sk_tomgr = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sk_tomgr < 0) {
        perror("Can't create socket");
        return -1;
    }
    struct sockaddr_in srvaddr;
    debugf("Connecting to %s:%d\n", CON_MGR_IP, CON_MGR_PORT);
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    ret = inet_aton(CON_MGR_IP, &srvaddr.sin_addr);
    if (ret == 0) {
        perror("Can't convert addr");
        return -1;
    }
    srvaddr.sin_port = htons(CON_MGR_PORT);
    ret = orig_connect_func(sk_tomgr, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
    if (ret < 0) {
        perror("Can't connect");
        return -1;
    }

    //Check for leader ship
    uint8_t iamleader = 0;
    ret = send_bytes(sk_tomgr, (char*)&iamleader, 1);
    char *buffer;
    int recv_len; 
    ret = recv_bytes(sk_tomgr, &buffer, &recv_len);
    if (recv_len != 1){
        perrorf("Failed to check for leadership, recv with len %d", recv_len);
        return -1;
    }
    iamleader = (uint8_t) buffer[0];
    free(buffer);

    debugf("[Intercept.so] I am leader? %d", iamleader);
    


    if(iamleader == 1){
        // Ask for consensus
        uint32_t send_seq, recv_seq;
        ret = tcp_repair_on(*sk);
        if (ret < 0){
            perrorf("Failed turn on");
        }
        ret = get_tcp_queue_seq(*sk, TCP_SEND_QUEUE, &send_seq);
        if (ret < 0){
            perrorf("Failed to get tcp_seq");
            return -1;
        }
        ret = get_tcp_queue_seq(*sk, TCP_RECV_QUEUE, &recv_seq);
        if (ret < 0){
            perrorf("Failed to get tcp_seq");
            return -1;
        }


        struct con_info_type *con_info;
        con_info = (struct con_info_type*) malloc(sizeof(struct con_info_type));
        memset(con_info, 0, sizeof(struct con_info_type));

        con_info->con_type = ACCEPT_CON;



        struct tcp_info tcpi;
        memset(&tcpi, 0, sizeof(struct tcp_info));
        int tcp_info_len = sizeof(struct tcp_info);



        ret = getsockopt(*sk, SOL_TCP, TCP_INFO, &tcpi, &tcp_info_len);
        if (ret != 0){
            perrorf("Fail to get TCP_INFO\n\n");
            return -1;
        }


        uint32_t timestamp = 0;
        uint8_t has_timestamp = 0;
        if (tcpi.tcpi_options & TCPI_OPT_TIMESTAMPS){
            has_timestamp = 1;
            int tslen = sizeof(timestamp);
            ret = getsockopt(*sk, SOL_TCP, TCP_TIMESTAMP, &timestamp, &tslen); 
            if (ret != 0){
                perrorf("Failed to get TCP_TIMESTAMP");
                return -1;
            }
        }
        con_info->has_timestamp = has_timestamp;
        con_info->timestamp = timestamp;


        if (tcpi.tcpi_options & TCPI_OPT_WSCALE){
            con_info->snd_wscale = tcpi.tcpi_snd_wscale; 
            con_info->rcv_wscale = tcpi.tcpi_rcv_wscale;
            con_info->has_rcv_wscale = 1;
        }

        debugf("snd_wscale: %"PRIu32", recv_wscale: %"PRIu32"\n\n", con_info->snd_wscale, con_info->rcv_wscale);

        //Get max seg size 

        int size_mss = sizeof(con_info->mss_clamp);

        ret = getsockopt(*sk, SOL_TCP, TCP_MAXSEG, &(con_info->mss_clamp), &size_mss);
        if (ret < 0){
            perrorf("Faailed to get mss_clamp\n\n");
            return -1;

        }

        debugf("mss_size: %"PRIu32"\n\n", con_info->mss_clamp);





        debugf("[LD_PRELOAD] send_seq: %"PRIu32"recv_seq: %"PRIu32"", send_seq, recv_seq);



        ret = tcp_repair_off(*sk);
        if (ret < 0){
            perrorf("Failed turn off");
        }
        
       
        get_tcp_con_id(*sk, con_info);
        con_info->send_seq = send_seq;
        con_info->recv_seq = recv_seq;
        ret = ask_for_consensus(sk_tomgr, con_info);
        if (ret < 0){
            perrorf("Failed to ask for consensus");
        }
    }
    else{
        struct con_info_type *con_info;
        con_info = (struct con_info_type*)malloc(sizeof(struct con_info_type));
        memset(con_info, 0, sizeof(struct con_info_type));

        char *in_buf;
        int len;  
        recv_bytes(*sk, &in_buf, &len);
        ret = con_info_deserialize(con_info, in_buf, len);
        if (ret < 0){
            perrorf("Failed to deserialize");
        }
        ret =replace_tcp(sk, con_info);
        if (ret < 0){
            perrorf("Failed to replace tcp");
        }

    }
    // int buffer_size; 
    // int bl = sizeof(buffer_size);
    // ret = getsockopt(sk, SOL_SOCKET, SO_RCVBUF, &buffer_size, &bl);
    // if (ret != 0){
    //     perrorf ("Failed to get rcv buffer_size\n");
    // }
    // else{
    //     debugf("Buffer size of recv buffer: %d\n", buffer_size);
    // }


    // ret = getsockopt(sk, SOL_SOCKET, SO_SNDBUF, &buffer_size, &bl);
    // if (ret != 0){
    //     perrorf ("Failed to get snd buffer_size\n");
    // }
    // else{
    //     debugf("Buffer size of snd buffer: %d\n", buffer_size);
    // }

    close(sk_tomgr);
    return 0;
}




int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    int ret, port;
    debugf("accept func intercepted on listening sk = %d", sockfd);
    int aux; 
    socklen_t len;

    orig_accept_func_type orig_accept_func; 
    orig_accept_func = (orig_accept_func_type) dlsym(RTLD_NEXT, "accept");
    int sk; 
    
    sk = orig_accept_func(sockfd, addr, addrlen);
    if (sk < 0){
        return sk; 
    }
    
    struct sockaddr_in localaddr;
    int addr_length = sizeof(localaddr);
    ret = getsockname(sk, (struct sockaddr*)&localaddr, &addr_length);

    debugf("Accept on port %u returned sk = %d",ntohs(localaddr.sin_port), sk);

    if (ntohs(localaddr.sin_port) == CON_MGR_PORT){
        debugf("Accept called by guest daemon, no need to hook");
        return sk; 
    }


    //uint32_t seq=0; 
    
    ret = handle_accepted_sk(&sk);
    if (ret < 0){
        perrorf("[Intercept.so] Accept function failed");
        return -1;
    }

    debugf("[Intercept.so] Return sk numebr sk= %d", sk);
    return sk; 
}








int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags){
    int ret, port;
    debugf("accept4 func intercepted on listening  sk = %d", sockfd);
    int aux; 
    socklen_t len;

    orig_accept4_func_type orig_accept4_func; 
    orig_accept4_func = (orig_accept4_func_type) dlsym(RTLD_NEXT, "accept4");
    int sk; 
    
    sk = orig_accept4_func(sockfd, addr, addrlen, flags);
    if (sk < 0){
        return sk; 
    }
    
    ret = handle_accepted_sk(&sk);
    if (ret < 0){
        perrorf("[Intercept.so] Accept function failed");
        return -1;
    }

    debugf("[Intercept.so] Return sk numebr sk= %d", sk);
    return sk; 

}




int socket(int domain, int type, int protocol){
    // printf("socket func called:");
    orig_socket_func_type orig_socket;
    orig_socket = (orig_socket_func_type) dlsym(RTLD_NEXT,"socket");
    int sk =  orig_socket(domain, type, protocol);
    int aux, ret;
    aux = 1;
    ret = setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &aux, sizeof(aux));
    if (ret < 0){
        perror("failed to setup reuse");
        return -1;
    }
    ret = setsockopt(sk, SOL_SOCKET, SO_REUSEPORT, &aux, sizeof(aux));
    if (ret < 0){
        perror("failed to setup reuse");
        return -1;
    }

    
    return sk;

}

int connect (int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    //Create a connection to manager to check whether it is leader
    
    orig_connect_func_type orig_connect_func; 
    orig_connect_func = (orig_connect_func_type) dlsym(RTLD_NEXT, "connect");



    int ret; 

    struct sockaddr_in *sin = (struct sockaddr_in *) addr; 

    struct in_addr sin_addr; 
    inet_aton(CON_MGR_IP, &sin_addr);



    struct in_addr my_addr; 
    inet_aton(MY_IP, &my_addr);

    if (ntohs(sin->sin_port) == QEMU_PORT || ntohs(sin->sin_port) == CON_MGR_PORT || sin->sin_addr.s_addr == sin_addr.s_addr || sin->sin_addr.s_addr == my_addr.s_addr){
        debugf("Local pair, no need to hook");
        int ret = orig_connect_func(sockfd, addr, addrlen);
        if (ret < 0){
            perror("System connect function failed with : ");
        }
        return ret; 
    }







    int sk_tomgr = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sk_tomgr < 0) {
        perror("Can't create socket");
        return -1;
    }
    struct sockaddr_in srvaddr;
    debugf("Connecting to %s:%d\n", CON_MGR_IP, CON_MGR_PORT);
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    ret = inet_aton(CON_MGR_IP, &srvaddr.sin_addr);
    if (ret == 0) {
        perror("Can't convert addr");
        return -1;
    }
    srvaddr.sin_port = htons(CON_MGR_PORT);
    ret = orig_connect_func(sk_tomgr, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
    if (ret < 0) {
        perror("Can't connect");
        return -1;
    }

    uint8_t iamleader = 0;
    ret = send_bytes(sk_tomgr, (char*)&iamleader, 1);
    char *buffer;
    int recv_len; 
    ret = recv_bytes(sk_tomgr, &buffer, &recv_len);
    if (recv_len != 1){
        perrorf("Failed to check for leadership, recv with len %d", recv_len);
        return -1;
    }
    iamleader = (uint8_t) buffer[0];
    free(buffer);

    debugf("[Intercept.so] [Connect] I am leader? %d", iamleader);


    
    if (iamleader == 1){
        //Connect, make consensus, return
        int ret = orig_connect_func(sockfd, addr, addrlen);
        if (ret < 0){
            if (errno == EINPROGRESS){
                sleep(1);
            }
            else{
                perror("System connect function failed with : ");
                return ret;
            }
        }


        //Get information out

        uint32_t send_seq, recv_seq;

        ret = tcp_repair_on(sockfd);
        if (ret < 0){
            perrorf("Failed turn on");
        }
        ret = get_tcp_queue_seq(sockfd, TCP_SEND_QUEUE, &send_seq);
        if (ret < 0){
            perrorf("Failed to get tcp_seq");
            return -1;
        }
        ret = get_tcp_queue_seq(sockfd, TCP_RECV_QUEUE, &recv_seq);
        if (ret < 0){
            perrorf("Failed to get tcp_seq");
            return -1;
        }


        struct con_info_type *con_info;
        con_info = (struct con_info_type*) malloc(sizeof(struct con_info_type));
        memset(con_info, 0, sizeof(struct con_info_type));

        con_info->con_type = CONNECT_CON;

        struct tcp_info tcpi;
        memset(&tcpi, 0, sizeof(struct tcp_info));
        int tcp_info_len = sizeof(struct tcp_info);



        ret = getsockopt(sockfd, SOL_TCP, TCP_INFO, &tcpi, &tcp_info_len);
        if (ret != 0){
            perrorf("Fail to get TCP_INFO\n\n");
            return -1;
        }

        uint32_t timestamp = 0;
        uint8_t has_timestamp = 0;
        if (tcpi.tcpi_options & TCPI_OPT_TIMESTAMPS){
            has_timestamp = 1;
            int tslen = sizeof(timestamp);
            ret = getsockopt(sockfd, SOL_TCP, TCP_TIMESTAMP, &timestamp, &tslen); 
            if (ret != 0){
                perrorf("Failed to get TCP_TIMESTAMP");
                return -1;
            }
        }
        con_info->has_timestamp = has_timestamp;
        con_info->timestamp = timestamp;


        if (tcpi.tcpi_options & TCPI_OPT_WSCALE){
            con_info->snd_wscale = tcpi.tcpi_snd_wscale; 
            con_info->rcv_wscale = tcpi.tcpi_rcv_wscale;
            con_info->has_rcv_wscale = 1;
        }

        debugf("snd_wscale: %"PRIu32", recv_wscale: %"PRIu32"\n\n", con_info->snd_wscale, con_info->rcv_wscale);

        //Get max seg size 

        int size_mss = sizeof(con_info->mss_clamp);

        ret = getsockopt(sockfd, SOL_TCP, TCP_MAXSEG, &(con_info->mss_clamp), &size_mss);
        if (ret < 0){
            perrorf("Faailed to get mss_clamp\n\n");
            return -1;

        }

        debugf("mss_size: %"PRIu32"\n\n", con_info->mss_clamp);





        debugf("[LD_PRELOAD] send_seq: %"PRIu32"recv_seq: %"PRIu32"", send_seq, recv_seq);



        ret = tcp_repair_off(sockfd);
        if (ret < 0){
            perrorf("Failed turn off");
        }


        get_tcp_con_id(sockfd, con_info);
        con_info->send_seq = send_seq;
        con_info->recv_seq = recv_seq;

        ret = ask_for_consensus(sk_tomgr, con_info);
        if (ret < 0){
            perrorf("Failed to ask for consensus");
        }

    }
    else{
        //Ask con-manager for the informations. 
        struct con_info_type *con_info = (struct con_info_type *) malloc(sizeof(struct con_info_type));
        memset(con_info, 0, sizeof(struct con_info_type));

        struct sockaddr_in *sin = (struct sockaddr_in *) addr; 

        con_info->con_id.dst_ip = sin->sin_addr.s_addr;
        con_info->con_id.dst_port = sin->sin_port;


        con_info->con_type = CONNECT_QUERY;


        char *buf;
        int len;
        ret = con_info_serialize(&buf, &len, con_info);
        if (ret < 0){
            perror("Failed to serialize connection info");
            return -1;
        }
        
        uint8_t success = 0;

        while (success == 0){
            sleep(1);
            ret = send_bytes(sk_tomgr, buf, len);
            if (ret < 0){
                perror("Failed to send con_info");
                return -1;
            }
            debugf("[Connect] Sent for asking for connect info , waiting for result");
            ret = recv(sk_tomgr, &success, sizeof(success), 0);
            if (success == 0)
                debugf("Not ready, will check for informations one second later");
        }

        
        free(buf);
        free(con_info);

        char *in_buf;
        int inlen; 


        ret = recv_bytes(sk_tomgr, &in_buf, &inlen);
        con_info = (struct con_info_type *) malloc(sizeof(struct con_info_type));


        ret = con_info_deserialize(con_info, in_buf, inlen);
        // create a socket according to the inforamtion. 

        ret = tcp_repair_on(sockfd);

        if (ret != 0){
            perrorf("Failed to turn on TCP Repair for :%d", sockfd);
            return ret;
        }
        ret = set_tcp_queue_seq(sockfd, TCP_RECV_QUEUE, con_info->recv_seq);
        if (ret != 0) {
            perror("Failed to set recv queue seq");
            return -1;
        }
        ret = set_tcp_queue_seq(sockfd, TCP_SEND_QUEUE, con_info->send_seq-1);
        if (ret != 0) {
            perror("Failed to set send queue seq");
            return -1;
        }

        int aux = 1;
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


        struct sockaddr_in localaddr, remoteaddr;


        memset(&localaddr, 0, sizeof(localaddr));
        localaddr.sin_family = AF_INET;
        localaddr.sin_port = con_info->con_id.src_port;
        localaddr.sin_addr.s_addr = con_info->con_id.src_ip; 



        memset(&remoteaddr, 0, sizeof(remoteaddr));
        remoteaddr.sin_family = AF_INET;
        remoteaddr.sin_port = con_info->con_id.dst_port;
        remoteaddr.sin_addr.s_addr = con_info->con_id.dst_ip; 





        if (bind(sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr)) != 0){
            perror("Cannot bind socket here");
            return -1;
        }

        if (orig_connect_func(sockfd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr)) != 0){
            perrorf("Cannot connect the socket to %s:%d", inet_ntoa(remoteaddr.sin_addr), ntohs(remoteaddr.sin_port));
            return -1;
        }

        debugf("[LD_PRELOAD] send_seq: %"PRIu32"recv_seq: %"PRIu32"", con_info->send_seq, con_info->recv_seq);

        if (con_info->has_timestamp){
            debugf("[LD_PRELOAD] Will turn TCP TIMESTAMP on, timestamp = %"PRIu32"", con_info->timestamp);
            struct tcp_repair_opt opt;
            opt.opt_code = TCPOPT_TIMESTAMP;
            opt.opt_val = 0;
            ret = setsockopt(sockfd, SOL_TCP, TCP_REPAIR_OPTIONS, &opt, sizeof(struct tcp_repair_opt));
            if (ret < 0){
                perrorf("Failed to repair TCP OPTIONS");
                return -1;
            }
            ret = setsockopt(sockfd, SOL_TCP, TCP_TIMESTAMP, &(con_info->timestamp), sizeof(con_info->timestamp));
            if (ret < 0){
                perrorf("Failed to set tcp timestamp");
                return -1;
            }
        }

        struct tcp_repair_opt opt_for_mss;  
        opt_for_mss.opt_code = TCPOPT_MAXSEG;
        opt_for_mss.opt_val = con_info->mss_clamp;

        ret = setsockopt(sockfd, SOL_TCP, TCP_REPAIR_OPTIONS, &opt_for_mss, sizeof(struct tcp_repair_opt));
        if (ret < 0){
            perrorf("Failed to set mss_clamp");
            return -1;
        }
        
        debugf("mss_size: %"PRIu32"", con_info->mss_clamp);




        if (con_info->has_rcv_wscale){
            struct tcp_repair_opt opt_for_wscale;
            opt_for_wscale.opt_code = TCPOPT_WINDOW;

            opt_for_wscale.opt_val = con_info->snd_wscale + (con_info->rcv_wscale << 16);
            ret = setsockopt(sockfd, SOL_TCP, TCP_REPAIR_OPTIONS, &opt_for_wscale, sizeof(struct tcp_repair_opt));
            if (ret < 0){
                perrorf("Faield to set wscale");
                return -1;
            } 
            debugf("snd_wscale: %"PRIu32", recv_wscale: %"PRIu32"", con_info->snd_wscale, con_info->rcv_wscale);

        }


        debugf("(Backup) created socket to %s:%"PRIu16"", inet_ntoa(remoteaddr.sin_addr), ntohs(remoteaddr.sin_port));
        
        uint8_t triger = 1;
        ret = send(sockfd, &triger, 1, 0);
        debugf("[Intercept.so]  Sent out 1 bytes for trigering");

        tcp_repair_off(sockfd);

    }
    close(sk_tomgr);


}