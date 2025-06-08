#ifndef PARSE_FILES_H
#define PARSE_FILES_H

// Estructura para IPv4 (tcp, udp)
typedef struct {
    char local_ip[16];
    int local_port;
    char remote_ip[16];
    int remote_port;
    int state;
    int uid;
    unsigned long inode;
} FileEntry4;

// Estructura para IPv6 (tcp6, udp6)
typedef struct {
    char local_ip[40];
    int local_port;
    char remote_ip[40];
    int remote_port;
    int state;
    int uid;
    unsigned long inode;
} FileEntry6;

// Funciones para parsear cada archivo
void parse_file4(int flag, FileEntry4 *entries, int *count); // 0:tcp, 1:udp
void parse_file6(int flag, FileEntry6 *entries, int *count); // 0:tcp6, 1:udp6

#endif