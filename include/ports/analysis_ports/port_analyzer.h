#ifndef PORT_ANALYZER_H
#define PORT_ANALYZER_H

#include "ports/parse_status_ports/parse_files.h"
#include "ports/ports.h"

// Analizar los puertos abiertos y conexiones activas contra la configuracion
void analyze_ports(
    const FileEntry4 *tcp, int tcp_count,
    const FileEntry4 *udp, int udp_count,
    const FileEntry6 *tcp6, int tcp6_count,
    const FileEntry6 *udp6, int udp6_count,
    const PortGuardConfig *config
);

#endif // PORT_ANALYZER_H