#include <sys/socket.h>
#include <linux/types.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <linux/sockios.h> // for SIOCINQ
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <inttypes.h>
#include <stdint.h>
#include "xmalloc.h"
#include <sys/ioctl.h>


typedef uint32_t u32;

int set_sequence (int sk);

static int serve_new_conn(int sk)
{
    int rd, wr;
    char buf[1024];

    printf("New connection\n");
    int ret = 0;
    // ret = set_sequence (sk);
    // if (ret < 0) {
    //     return -1;
    // }

    



    while (1) {
        rd = read(sk, buf, sizeof(buf));
        if (!rd)
            break;

        if (rd < 0) {
            perror("Can't read socket");
            return 1;
        }

        wr = 0;
        while (wr < rd) {
            int w;

            w = write(sk, buf + wr, rd - wr);
            if (w <= 0) {
                perror("Can't write socket");
                return 1;
            }

            wr += w;
        }
    }

    printf("Done\n");
    return 0;
}

static int main_srv(int argc, char **argv)
{
    int sk, port, ret;
    struct sockaddr_in addr;

    /*
     * Let kids die themselves
     */

    signal(SIGCHLD, SIG_IGN);

    sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sk < 0) {
        perror("Can't create socket");
        return -1;
    }





    port = atoi(argv[1]);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    printf("Binding to port %d\n", port);




    ret = bind(sk, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("Can't bind socket");
        return -1;
    }



    ret = listen(sk, 16);
    if (ret < 0) {
        perror("Can't put sock to listen");
        return -1;
    }



    printf("Waiting for connections\n");
    while (1) {
        int ask, pid;





        ask = accept(sk, NULL, NULL);
        





        if (ask < 0) {
            perror("Can't accept new conn");
            return -1;
        }

        pid = fork();
        if (pid < 0) {
            perror("Can't fork");
            return -1;
        }

        if (pid > 0)
            close(ask);
        else {
            close(sk);
            ret = serve_new_conn(ask);
            exit(ret);
        }
    }
}

static int main_cl(int argc, char **argv)
{
    int sk, port, ret, val = 1, rval;
    struct sockaddr_in addr;

    sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sk < 0) {
        perror("Can't create socket");
        return -1;
    }

    port = atoi(argv[2]);
    printf("Connecting to %s:%d\n", argv[1], port);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    ret = inet_aton(argv[1], &addr.sin_addr);
    if (ret < 0) {
        perror("Can't convert addr");
        return -1;
    }
    addr.sin_port = htons(port);








    ret = connect(sk, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("Can't connect");
        return -1;
    }

    // int aux = 1;
    // ret = setsockopt(sk, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
    //     if (ret < 0) {
    //         perror("Cannot turn on TCP_REPAIR mode");
    //     }
    // socklen_t auxl;
    // int queue_id = TCP_SEND_QUEUE;
    // auxl = sizeof(queue_id);
    // ret = setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue_id, auxl);
    // if (ret < 0) {
    //     perror("Failed to set to TCP_REPAIR_QUEUE for sentqueue");
    // }
    // u32 seq = 931209;
    // ret = setsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, &seq, auxl);
    // if (ret < 0) {
    //     perror("Failed to get the TCP send queue seq");
    // }
    // printf("queue id: %" PRIu32 "\n", seq);

    // aux = 0;
    // ret = setsockopt(sk, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
    //     if (ret < 0) {
    //         perror("Cannot turn on TCP_REPAIR mode");
    //     }





    //u32 seq;
    //socklen_t tcp_info_length = sizeof(struct tcp_info);
    while (1) {
        // int aux = 1;
        // //enter repari mode
        // ret = setsockopt(sk, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
        // if (ret < 0) {
        //     perror("Cannot turn on TCP_REPAIR mode");
        // }

        // socklen_t auxl;
        // int queue_id = TCP_SEND_QUEUE;
        // auxl = sizeof(queue_id);
        // ret = setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue_id, auxl);
        // if (ret < 0) {
        //     perror("Failed to set to TCP_REPAIR_QUEUE for sentqueue");
        // }
        // u32 seq;
        // ret = getsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, &seq, &auxl);
        // if (ret < 0) {
        //     perror("Failed to get the TCP send queue seq");
        // }
        // printf("queue id: %" PRIu32 "\n", seq);
        //struct tcp_info info;
        //int queue_id;
        //socklen_t auxl;
        //auxl = sizeof(queue_id);


        //ret = getsockopt(sk,SOL_TCP,TCP_QUEUE_SEQ,&seq,&auxl);
        //if (ret != 0){
        //    perror("getscokopt");
        //    return -1;
        //}
        // printf("Got opt:%s, length:%d\n", info,tcp_info_length);
        //printf("Value:%" PRIu32 "\n", info.tcpi_last_data_sent);
        write(sk, &val, sizeof(val));
        rval = -1;
        read(sk, &rval, sizeof(rval));
        printf("PP %d -> %d\n", val, rval);
        


        sleep(2);
        val++;
    }
}

/**
static int tcp_repair_on(int fd)
{
    int ret, aux = 1;

    ret = setsockopt(fd, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
    if (ret < 0)
        pr_perror("Can't turn TCP repair mode ON");
     return ret;
}
**/





int main(int argc, char **argv)
{
    if (argc == 2)
        return main_srv(argc, argv);
    else if (argc == 3)
        return main_cl(argc, argv);

    printf("Bad usage\n");
    return 1;
}


int set_sequence (int sk) {
    int aux = 1;
    int ret = 0; 
    u32 seq = 931209;
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
    ret = setsockopt(sk, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
        if (ret < 0) {
            perror("Cannot turn on TCP_REPAIR mode");
            return ret;
        }

}




static int tcp_repair_on(int fd)
{
    int ret, aux = 1;

    ret = setsockopt(fd, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
    if (ret < 0)
        perror("Can't turn TCP repair mode ON");

    return ret;
}

static int tcp_stream_get_queue(int sk, int queue_id,
        u32 *seq, u32 len, char **bufp)
{
    int ret, aux;
    socklen_t auxl;
    char *buf;

    //pr_debug("\tSet repair queue %d\n", queue_id);
    aux = queue_id;
    auxl = sizeof(aux);
    ret = setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &aux, auxl);
    if (ret < 0){
        perror("Failed to get TCP Queue 1");
        return ret;
    }

    //pr_debug("\tGet queue seq\n");
    auxl = sizeof(*seq);
    ret = getsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, seq, &auxl);
    if (ret < 0){
        perror("Failed to get TCP Queue 1");
        return ret;
    }


    printf("\t`- seq %u len %u\n", *seq, len);

    if (len) {
        /*
         * Try to grab one byte more from the queue to
         * make sure there are len bytes for real
         */
        buf = xmalloc(len + 1);
        if (!buf){
            perror("buffer failed xmalloc");
            return -1;
        }

        ret = recv(sk, buf, len + 1, MSG_PEEK | MSG_DONTWAIT);
        if (ret != len){
            perror("Error recv");
            return -1;
        }
    } else
        buf = NULL;

    *bufp = buf;
    return 0;
}


// typedef struct _TcpStreamEntry TcpStreamEntry;


// /* --- enums --- */


// /* --- messages --- */

// struct  _TcpStreamEntry
// {
//   ProtobufCMessage base;
//   uint32_t inq_len;
//   uint32_t inq_seq;
//   /*
//    * unsent and sent data in the send queue
//    */
//   uint32_t outq_len;
//   uint32_t outq_seq;
//   /*
//    * TCPI_OPT_ bits 
//    */
//   uint32_t opt_mask;
//   uint32_t snd_wscale;
//   uint32_t mss_clamp;
//   protobuf_c_boolean has_rcv_wscale;
//   uint32_t rcv_wscale;
//   protobuf_c_boolean has_timestamp;
//   uint32_t timestamp;
//   protobuf_c_boolean has_cork;
//   protobuf_c_boolean cork;
//   protobuf_c_boolean has_nodelay;
//   protobuf_c_boolean nodelay;
//   /*
//    * unsent data in the send queue 
//    */
//   protobuf_c_boolean has_unsq_len;
//   uint32_t unsq_len;
//   protobuf_c_boolean has_snd_wl1;
//   uint32_t snd_wl1;
//   protobuf_c_boolean has_snd_wnd;
//   uint32_t snd_wnd;
//   protobuf_c_boolean has_max_window;
//   uint32_t max_window;
//   protobuf_c_boolean has_rcv_wnd;
//   uint32_t rcv_wnd;
//   protobuf_c_boolean has_rcv_wup;
//   uint32_t rcv_wup;
// };

//from CRIU socket.h 
int do_dump_opt(int sk, int level, int name, void *val, int len)
{
    socklen_t aux = len;

    if (getsockopt(sk, level, name, val, &aux) < 0) {
        printf("Can't get %d:%d opt", level, name);
        return -1;
    }

    if (aux != len) {
        printf("Len mismatch on %d:%d : %d, want %d\n",
                level, name, aux, len);
        return -1;
    }

    return 0;
}
#define dump_opt(s, l, n, f)    do_dump_opt(s, l, n, f, sizeof(*f))

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



int inet_bind(int sk, addr){
    
}

int replace_tcp (int *sk){
    int ret, aux; 
    struct tcp_info ti; 
    
    // turn on Repair mode
    ret = tcp_repair_on(*sk);
    if (ret != 0){
        return ret;
    }

    char *in_buf, *out_buf;
    u32 inq_seq, outq_seq;
    u32 inq_len;
    u32 outq_len, unsq_len;

    // The information got from refresh_inet_sk in criu
    unsigned int rqlen, wqlen, uwqlen;

    int size; 
    if (ioctl(*sk, SIOCOUTQ, &size) == -1) {
        perror("Unable to get size of snd queue");
        return -1;
    }

    wqlen = size;
    if (ioctl(*sk, SIOCOUTQNSD, &size) == -1) {
        perror("Unable to get size of unsent data");
        return -1;
    }
    uwqlen = size;

    if (ioctl(*sk, SIOCINQ, &size) == -1) {
        perror("Unable to get size of recv queue");
        return -1;
    }
    rqlen = size;


    /*
     * Read queue
     */

    inq_len = rqlen; 
    ret = tcp_stream_get_queue(*sk, TCP_RECV_QUEUE, &inq_seq, inq_len, &in_buf);
    if (ret < 0){
        perror("Failed to get the RECV QUEUE");
        return -1;
    }

    /*
     * Write queue
     */

    outq_len = wqlen;
    unsq_len = uwqlen;

    ret = tcp_stream_get_queue(*sk, TCP_SEND_QUEUE, &outq_seq, outq_len, &out_buf);
    if (ret < 0){
        perror("Failed to get the Send QUEUE");
        return -1;
    }

    //TODO: Window size;

    //options
    int has_nodelay = 0; 
    int nodelay = 0;
    int has_cork = 0; 
    int cork = 0;



    if (dump_opt(*sk, SOL_TCP, TCP_NODELAY, &aux)){
        printf("failed to get the TCP_NODELAY opt\n");
        return -1;
    }

    if (aux) {
        has_nodelay = 1;
        nodelay = 1;
    }

    if (dump_opt(*sk, SOL_TCP, TCP_CORK, &aux)){
        printf("faied to get the TCP_CORK opt\n");
        return -1;
    }

    if (aux) {
        has_cork = 1;
        cork = 1;
    }

    shutdown(*sk, 0);

    int sk_new; 
    sk_new = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Repair mode on
    ret = tcp_repair_on(sk_new);
    if (ret != 0){
        return ret;
    }

    //Restore tcp seq
    ret = set_tcp_queue_seq(sk_new, TCP_RECV_QUEUE, inq_seq - inq_len);
    if (ret != 0) {
        perror("Failed to set recv queue");
        return -1;
    }

    ret = set_tcp_queue_seq(sk_new, TCP_SEND_QUEUE, outq_seq - outq_len);
    if (ret != 0) {
        perror("Failed to set send queue");
        return -1;
    }

}