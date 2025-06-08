#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "ports/parse_status_ports/parse_files.h"

// Convertir IP de hex a string decimal (IPv4)
void hex_to_ip4(const char *hex, char *ip_str) {
    unsigned int ip[4];
    sscanf(hex, "%2X%2X%2X%2X", &ip[3], &ip[2], &ip[1], &ip[0]);
    sprintf(ip_str, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

// Convertir IP de hex a string (IPv6)
void hex_to_ip6(const char *hex, char *ip_str) {
    unsigned char ip_bytes[16];
    char block[5] = {0};
    for (int i = 0; i < 16; ++i) {
        strncpy(block, hex + i * 2, 2);
        ip_bytes[i] = (unsigned char)strtol(block, NULL, 16);
    }
    inet_ntop(AF_INET6, ip_bytes, ip_str, 40);
}

// Parsear una linea de TCP/UDP IPv4
int parse_tcp_line4(const char *line, FileEntry4 *entry) {
    char local_ip_hex[9], rem_ip_hex[9];
    unsigned int local_port_hex, rem_port_hex;
    int state, uid;
    unsigned long inode;

    int fields = sscanf(
        line,
        "%*d: %8[0-9A-Fa-f]:%X %8[0-9A-Fa-f]:%X %X %*s %*s %*s %d %*s %lu",
        local_ip_hex, &local_port_hex, rem_ip_hex, &rem_port_hex, &state, &uid, &inode
    );

    if (fields < 7) return 0;

    hex_to_ip4(local_ip_hex, entry->local_ip);
    hex_to_ip4(rem_ip_hex, entry->remote_ip);
    entry->local_port = local_port_hex;
    entry->remote_port = rem_port_hex;
    entry->state = state;
    entry->uid = uid;
    entry->inode = inode;

    return 1;
}

// Parsear una linea de TCP/UDP IPv6
int parse_tcp_line6(const char *line, FileEntry6 *entry) {
    char local_ip_hex[33], rem_ip_hex[33];
    unsigned int local_port_hex, rem_port_hex;
    int state, uid;
    unsigned long inode;

    int fields = sscanf(
        line,
        "%*d: %32[0-9A-Fa-f]:%X %32[0-9A-Fa-f]:%X %X %*s %*s %*s %d %*s %lu",
        local_ip_hex, &local_port_hex, rem_ip_hex, &rem_port_hex, &state, &uid, &inode
    );

    if (fields < 7) return 0;

    hex_to_ip6(local_ip_hex, entry->local_ip);
    hex_to_ip6(rem_ip_hex, entry->remote_ip);
    entry->local_port = local_port_hex;
    entry->remote_port = rem_port_hex;
    entry->state = state;
    entry->uid = uid;
    entry->inode = inode;

    return 1;
}

// Parsear archivo TCP o UDP IPv4
void parse_file4(int flag, FileEntry4 *entries, int *count) {
    const char *paths[] = { "/proc/net/tcp", "/proc/net/udp" };
    if (flag < 0 || flag > 1) {
        fprintf(stderr, "Flag inválido para parse_file4\n");
        *count = 0;
        return;
    }
    FILE *file = fopen(paths[flag], "r");
    if (!file) {
        perror("No se pudo abrir el archivo de red");
        *count = 0;
        return;
    }
    char line[512];
    fgets(line, sizeof(line), file); // Saltar encabezado
    *count = 0;
    while (fgets(line, sizeof(line), file) && *count < 256) {
        FileEntry4 entry;
        if (parse_tcp_line4(line, &entry)) {
            entries[(*count)++] = entry;
        }
    }
    fclose(file);
}

// Parsear archivo TCP6 o UDP6 IPv6
void parse_file6(int flag, FileEntry6 *entries, int *count) {
    const char *paths[] = { "/proc/net/tcp6", "/proc/net/udp6" };
    if (flag < 0 || flag > 1) {
        fprintf(stderr, "Flag inválido para parse_file6\n");
        *count = 0;
        return;
    }
    FILE *file = fopen(paths[flag], "r");
    if (!file) {
        perror("No se pudo abrir el archivo de red");
        *count = 0;
        return;
    }
    char line[512];
    fgets(line, sizeof(line), file); // Saltar encabezado
    *count = 0;
    while (fgets(line, sizeof(line), file) && *count < 256) {
        FileEntry6 entry;
        if (parse_tcp_line6(line, &entry)) {
            entries[(*count)++] = entry;
        }
    }
    fclose(file);
}