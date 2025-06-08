#ifndef READ_TCP_H
#define READ_TCP_H

typedef struct {
    char local_ip[16];
    int local_port;
    char remote_ip[16];
    int remote_port;
    int state;
    int uid;
    unsigned long inode;
} TcpEntry;

void parse_tcp();
#endif