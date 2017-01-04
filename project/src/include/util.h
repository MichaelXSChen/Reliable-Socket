#ifndef UTIL_H
#define UTIL_H

int recv_bytes(int sk, char** buf, int *length);
int send_bytes(int sk, char* buf, int len);

#endif