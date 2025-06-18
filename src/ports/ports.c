#include "ports.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

// =======================================================
// Configuración estática de servicios y puertos permitidos
// =======================================================

typedef struct {
    int port;
    const char *service;
} PortService;

static PortService common_services[] = {
    {21, "FTP"},    {22, "SSH"},    {23, "Telnet"}, {25, "SMTP"},    {53, "DNS"},
    {67, "DHCP"},   {68, "DHCP"},   {80, "HTTP"},   {110, "POP3"},   {111, "RPCbind"},
    {123, "NTP"},   {135, "MS RPC"},{139, "NetBIOS"},{143, "IMAP"},  {161, "SNMP"},
    {179, "BGP"},   {389, "LDAP"},  {443, "HTTPS"}, {445, "SMB"},    {465, "SMTPS"},
    {514, "Syslog"},{515, "LPD"},   {587, "Submission"},{631, "IPP/CUPS"},{993, "IMAPS"},
    {995, "POP3S"}, {1433, "MSSQL"},{1521, "Oracle"},{2049, "NFS"},  {3306, "MySQL"},
    {3389, "RDP"},  {5432, "PostgreSQL"},{5900, "VNC"},{8080, "HTTP-alt"},{8443, "HTTPS-alt"},
    {8888, "Alternate HTTP"}, {6667, "IRC"}, {31337, "Back Orifice/Elite"}, {4444, "Metasploit/Backdoor"}
};

static const char *get_service_name(int port) {
    size_t len = sizeof(common_services) / sizeof(common_services[0]);
    for (size_t i = 0; i < len; ++i) {
        if (common_services[i].port == port)
            return common_services[i].service;
    }
    return NULL;
}

static int justified_high_ports[] = {8080, 8443, 8888};

static int is_justified_high_port(int port) {
    size_t len = sizeof(justified_high_ports) / sizeof(justified_high_ports[0]);
    for (size_t i = 0; i < len; ++i) {
        if (justified_high_ports[i] == port)
            return 1;
    }
    return 0;
}

static int suspicious_ports[] = {31337, 4444, 6667};

static int is_suspicious_port(int port) {
    size_t len = sizeof(suspicious_ports) / sizeof(suspicious_ports[0]);
    for (size_t i = 0; i < len; ++i) {
        if (suspicious_ports[i] == port)
            return 1;
    }
    return 0;
}

// =======================================================
// Almacenamiento del estado previo
// =======================================================
#define START_PORT 0
#define END_PORT 65535
#define NUM_PORTS (END_PORT - START_PORT + 1)
static int previous_states[NUM_PORTS];

void init_previous_states() {
    for (int i = 0; i < NUM_PORTS; i++) {
        previous_states[i] = -1;
    }
}

// =======================================================
// Escaneo TCP con impresión por rangos
// =======================================================
void scan_ports_tcp(int start_port, int end_port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Escaneando puertos TCP del %d al %d en localhost...\n", start_port, end_port);

    int start_closed = -1;
    int in_closed_block = 0;

    for (int port = start_port; port <= end_port; ++port) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
            continue;

        struct timeval timeout = {1, 0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        addr.sin_port = htons(port);
        int result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        close(sock);

        int new_status = (result == 0) ? 1 : 0;
        int index = port - start_port;

        if (new_status == 0) {
            // puerto cerrado
            if (!in_closed_block) {
                start_closed = port;
                in_closed_block = 1;
            }
        } else {
            // si se estaba acumulando cerrados, imprimir bloque
            if (in_closed_block) {
                if (start_closed == port - 1)
                    printf("Reporte: \"Puerto %d/tcp cerrado\"\n", start_closed);
                else
                    printf("Reporte: \"Puertos %d–%d/tcp cerrados\"\n", start_closed, port - 1);
                in_closed_block = 0;
            }

            if (previous_states[index] != 1) {
                const char *service = get_service_name(port);
                if (is_suspicious_port(port)) {
                    if (port == 4444)
                        printf("Alerta: \"Servidor HTTP en puerto no estándar %d/tcp\"\n", port);
                    else
                        printf("Alerta: \"Puerto %d/tcp abierto (no permitido)\"\n", port);
                } else {
                    if (service == NULL && port > 1024 && !is_justified_high_port(port))
                        printf("Alerta: \"Puerto %d/tcp abierto (no permitido, servicio desconocido)\"\n", port);
                    else {
                        if (service != NULL)
                            printf("Reporte: \"Puerto %d/tcp (%s) abierto (permitido)\"\n", port, service);
                        else
                            printf("Reporte: \"Puerto %d/tcp abierto (permitido)\"\n", port);
                    }
                }
                previous_states[index] = 1;
            }
        }

        if (new_status != previous_states[index])
            previous_states[index] = new_status;
    }

    // Imprimir bloque final si quedaron puertos cerrados al final
    if (in_closed_block) {
        if (start_closed == end_port)
            printf("Reporte: \"Puerto %d/tcp cerrado\"\n", start_closed);
        else
            printf("Reporte: \"Puertos %d–%d/tcp cerrados\"\n", start_closed, end_port);
    }
}
