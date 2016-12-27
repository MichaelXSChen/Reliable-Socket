#include <sys/socket.h>
#include <linux/types.h>
#include <sys/types.h>
#include <arpa/inet.h>
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

typedef uint32_t u32;


static int serve_new_conn(int sk)
{
    int rd, wr;
    char buf[1024];

    printf("New connection\n");

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

    ret = setsockopt(sk, SOL_TCP, TCP_REPAIR, &aux, sizeof(aux));
        if (ret < 0) {
            perror("Cannot turn on TCP_REPAIR mode");
        }
    socklen_t auxl;
    int queue_id = TCP_SEND_QUEUE;
    auxl = sizeof(queue_id);
    ret = setsockopt(sk, SOL_TCP, TCP_REPAIR_QUEUE, &queue_id, auxl);
    if (ret < 0) {
        perror("Failed to set to TCP_REPAIR_QUEUE for sentqueue");
    }
    u32 seq;
    ret = getsockopt(sk, SOL_TCP, TCP_QUEUE_SEQ, &seq, &auxl);
    if (ret < 0) {
        perror("Failed to get the TCP send queue seq");
    }
    printf("queue id: %" PRIu32 "\n", seq);






    ret = connect(sk, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("Can't connect");
        return -1;
    }
    u32 seq;
    socklen_t tcp_info_length = sizeof(struct tcp_info);
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
