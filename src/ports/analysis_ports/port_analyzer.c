#include <stdio.h>
#include <stdbool.h>
#include "ports/parse_status_ports/parse_files.h"
#include "ports/ports.h"
#include "ports/analysis_ports/process_lookup.h"

// Funcion auxiliar verificar si un puerto está permitido
static bool is_port_allowed(int port, const PortGuardConfig *config) {
    for (int i = 0; i < config->allowed_ports_count; ++i) {
        if (config->allowed_ports[i] == port)
            return true;
    }
    return false;
}

// Analizar puertos abiertos y conexiones activas
void analyze_ports(
    const FileEntry4 *tcp, int tcp_count,
    const FileEntry4 *udp, int udp_count,
    const FileEntry6 *tcp6, int tcp6_count,
    const FileEntry6 *udp6, int udp6_count,
    const PortGuardConfig *config
) {
    printf("\n--- ANALISIS DE PUERTOS ABIERTOS Y CONEXIONES ---\n");

    // Analizar TCP IPv4
    for (int i = 0; i < tcp_count; ++i) {
        if (!is_port_allowed(tcp[i].local_port, config)) {
            char proc_name[256];
            int pid;
            if (find_process_by_inode(tcp[i].inode, &pid, proc_name, sizeof(proc_name))) {
                printf("[ALERTA] Puerto TCP (IPv4) NO permitido: %s:%d (PID: %d, Proceso: %s, UID: %d, Inode: %lu)\n",
                    tcp[i].local_ip, tcp[i].local_port, pid, proc_name, tcp[i].uid, tcp[i].inode);
            } else {
                printf("[ALERTA] Puerto TCP (IPv4) NO permitido: %s:%d (PID: ?, Proceso: ?, UID: %d, Inode: %lu)\n",
                    tcp[i].local_ip, tcp[i].local_port, tcp[i].uid, tcp[i].inode);
            }
        }
    }

    // Analizar UDP IPv4
    for (int i = 0; i < udp_count; ++i) {
        if (!is_port_allowed(udp[i].local_port, config)) {
            char proc_name[256];
            int pid;
            if (find_process_by_inode(udp[i].inode, &pid, proc_name, sizeof(proc_name))) {
                printf("[ALERTA] Puerto UDP (IPv4) NO permitido: %s:%d (PID: %d, Proceso: %s, UID: %d, Inode: %lu)\n",
                    udp[i].local_ip, udp[i].local_port, pid, proc_name, udp[i].uid, udp[i].inode);
            } else {
                printf("[ALERTA] Puerto UDP (IPv4) NO permitido: %s:%d (PID: ?, Proceso: ?, UID: %d, Inode: %lu)\n",
                    udp[i].local_ip, udp[i].local_port, udp[i].uid, udp[i].inode);
            }
        }
    }

    // Analizar TCP IPv6
    for (int i = 0; i < tcp6_count; ++i) {
        if (!is_port_allowed(tcp6[i].local_port, config)) {
            char proc_name[256];
            int pid;
            if (find_process_by_inode(tcp6[i].inode, &pid, proc_name, sizeof(proc_name))) {
                printf("[ALERTA] Puerto TCP (IPv6) NO permitido: %s:%d (PID: %d, Proceso: %s, UID: %d, Inode: %lu)\n",
                    tcp6[i].local_ip, tcp6[i].local_port, pid, proc_name, tcp6[i].uid, tcp6[i].inode);
            } else {
                printf("[ALERTA] Puerto TCP (IPv6) NO permitido: %s:%d (PID: ?, Proceso: ?, UID: %d, Inode: %lu)\n",
                    tcp6[i].local_ip, tcp6[i].local_port, tcp6[i].uid, tcp6[i].inode);
            }
        }
    }

    // Analizar UDP IPv6
    for (int i = 0; i < udp6_count; ++i) {
        if (!is_port_allowed(udp6[i].local_port, config)) {
            char proc_name[256];
            int pid;
            if (find_process_by_inode(udp6[i].inode, &pid, proc_name, sizeof(proc_name))) {
                printf("[ALERTA] Puerto UDP (IPv6) NO permitido: %s:%d (PID: %d, Proceso: %s, UID: %d, Inode: %lu)\n",
                    udp6[i].local_ip, udp6[i].local_port, pid, proc_name, udp6[i].uid, udp6[i].inode);
            } else {
                printf("[ALERTA] Puerto UDP (IPv6) NO permitido: %s:%d (PID: ?, Proceso: ?, UID: %d, Inode: %lu)\n",
                    udp6[i].local_ip, udp6[i].local_port, udp6[i].uid, udp6[i].inode);
            }
        }
    }
}