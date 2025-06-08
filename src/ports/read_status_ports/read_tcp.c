#include <stdio.h>
#include <stdlib.h>
#include "ports/read_status_ports/read_tcp.h"

// Convertir IP de hex a string decimal
void hex_to_ip(char *hex, char *ip_str) {
    unsigned int ip[4];
    sscanf(hex, "%2X%2X%2X%2X", &ip[3], &ip[2], &ip[1], &ip[0]);
    sprintf(ip_str, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

// Parsear una linea de TCP
int parse_tcp_line(const char *line, TcpEntry *entry) {
    char local_addr[64], rem_addr[64];
    char local_ip_hex[9], rem_ip_hex[9];
    unsigned int local_port_hex, rem_port_hex;
    int state, uid;
    unsigned long inode;

    // El formato de sscanf debe coincidir con la estructura de /proc/net/tcp
    int fields = sscanf(
        line,
        "%*d: %8[0-9A-Fa-f]:%X %8[0-9A-Fa-f]:%X %X %*s %*s %*s %d %*s %lu",
        local_ip_hex, &local_port_hex, rem_ip_hex, &rem_port_hex, &state, &uid, &inode
    );

    if (fields < 7) return 0;

    hex_to_ip(local_ip_hex, entry->local_ip);
    hex_to_ip(rem_ip_hex, entry->remote_ip);
    entry->local_port = local_port_hex;
    entry->remote_port = rem_port_hex;
    entry->state = state;
    entry->uid = uid;
    entry->inode = inode;

    return 1;
}

// Abrir el archivo TCP en modo lectura
void parse_tcp() {
    FILE *file = fopen("/proc/net/tcp", "r");
    if (!file) {
        perror("No se pudo abrir /proc/net/tcp");
        exit(EXIT_FAILURE);
    }

    char line[512];
    fgets(line, sizeof(line), file); // Saltar encabezado

    TcpEntry entries[256];
    int count = 0;

    while (fgets(line, sizeof(line), file) && count < 256) {
        TcpEntry entry;
        if (parse_tcp_line(line, &entry)) {
            entries[count++] = entry; // Guardar entrada válida
        }
    }
    fclose(file);

    // Imprimir las entradas TCP encontradas
    for (int i = 0; i < count; i++) {
        printf("Local IP: %s:%d, Remote IP: %s:%d, State: %d, UID: %d, Inode: %lu\n",
               entries[i].local_ip, entries[i].local_port,
               entries[i].remote_ip, entries[i].remote_port,
               entries[i].state, entries[i].uid, entries[i].inode);
    }
}