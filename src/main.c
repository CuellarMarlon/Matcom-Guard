#include <stdio.h>
#include "ports/parse_status_ports/parse_files.h"
#include "ports/analysis_ports/port_analyzer.h"
#include "ports/ports.h"
#include "ports/scan_ports/active_scanner.h"

int main(void) {
    FileEntry4 tcp_entries[256], udp_entries[256];
    FileEntry6 tcp6_entries[256], udp6_entries[256];
    int tcp_count, udp_count, tcp6_count, udp6_count;

    // Leer los archivos de red
    parse_file4(0, tcp_entries, &tcp_count);
    parse_file4(1, udp_entries, &udp_count);
    parse_file6(0, tcp6_entries, &tcp6_count);
    parse_file6(1, udp6_entries, &udp6_count);

    // Leer la configuración
    PortGuardConfig config;
    read_config_file_example(&config); // Asegúrate de que tu función llene la estructura

    // Analizar los puertos y conexiones
    analyze_ports(
        tcp_entries, tcp_count,
        udp_entries, udp_count,
        tcp6_entries, tcp6_count,
        udp6_entries, udp6_count,
        &config
    );

    // Escaneo activo de puertos TCP del 1 al 1024
    scan_ports_tcp(1, 1024);

    return 0;
}