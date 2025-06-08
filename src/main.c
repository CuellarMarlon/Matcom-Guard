#include <stdio.h>
#include "ports/parse_status_ports/parse_files.h"

int main(void) {
    FileEntry4 tcp_entries[256], udp_entries[256];
    FileEntry6 tcp6_entries[256], udp6_entries[256];
    int tcp_count, udp_count, tcp6_count, udp6_count;

    printf("Archivo TCP (IPv4)\n");
    parse_file4(0, tcp_entries, &tcp_count);
    for (int i = 0; i < tcp_count; ++i)
        printf("Local: %s:%d, Remote: %s:%d, State: %d, UID: %d, Inode: %lu\n",
            tcp_entries[i].local_ip, tcp_entries[i].local_port,
            tcp_entries[i].remote_ip, tcp_entries[i].remote_port,
            tcp_entries[i].state, tcp_entries[i].uid, tcp_entries[i].inode);

    printf("\nArchivo UDP (IPv4)\n");
    parse_file4(1, udp_entries, &udp_count);
    for (int i = 0; i < udp_count; ++i)
        printf("Local: %s:%d, Remote: %s:%d, State: %d, UID: %d, Inode: %lu\n",
            udp_entries[i].local_ip, udp_entries[i].local_port,
            udp_entries[i].remote_ip, udp_entries[i].remote_port,
            udp_entries[i].state, udp_entries[i].uid, udp_entries[i].inode);

    printf("\nArchivo TCP6 (IPv6)\n");
    parse_file6(0, tcp6_entries, &tcp6_count);
    for (int i = 0; i < tcp6_count; ++i)
        printf("Local: %s:%d, Remote: %s:%d, State: %d, UID: %d, Inode: %lu\n",
            tcp6_entries[i].local_ip, tcp6_entries[i].local_port,
            tcp6_entries[i].remote_ip, tcp6_entries[i].remote_port,
            tcp6_entries[i].state, tcp6_entries[i].uid, tcp6_entries[i].inode);

    printf("\nArchivo UDP6 (IPv6)\n");
    parse_file6(1, udp6_entries, &udp6_count);
    for (int i = 0; i < udp6_count; ++i)
        printf("Local: %s:%d, Remote: %s:%d, State: %d, UID: %d, Inode: %lu\n",
            udp6_entries[i].local_ip, udp6_entries[i].local_port,
            udp6_entries[i].remote_ip, udp6_entries[i].remote_port,
            udp6_entries[i].state, udp6_entries[i].uid, udp6_entries[i].inode);

    return 0;
}