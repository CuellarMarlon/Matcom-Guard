#include "ports/scan_ports/active_scanner.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef struct {
    int port;
    const char *service;
} PortService;

static PortService common_services[] = {
    {21, "FTP"}, {22, "SSH"}, {23, "Telnet"}, {25, "SMTP"}, {53, "DNS"},
    {67, "DHCP"}, {68, "DHCP"}, {80, "HTTP"}, {110, "POP3"}, {111, "RPCbind"},
    {123, "NTP"}, {135, "MS RPC"}, {139, "NetBIOS"}, {143, "IMAP"},
    {161, "SNMP"}, {179, "BGP"}, {389, "LDAP"}, {443, "HTTPS"},
    {445, "SMB"}, {465, "SMTPS"}, {514, "Syslog"}, {515, "LPD"},
    {587, "Submission"}, {631, "IPP/CUPS"}, {993, "IMAPS"}, {995, "POP3S"},
    {1433, "MSSQL"}, {1521, "Oracle"}, {2049, "NFS"}, {3306, "MySQL"},
    {3389, "RDP"}, {5432, "PostgreSQL"}, {5900, "VNC"}, {8080, "HTTP-alt"},
    {8443, "HTTPS-alt"}, {8888, "Alternate HTTP"}, {6667, "IRC"},
    {31337, "Back Orifice/Elite"}, {4444, "Metasploit/Backdoor"}
    // Puedes agregar más según tus necesidades
};

static const char* get_service_name(int port) {
    for (size_t i = 0; i < sizeof(common_services)/sizeof(common_services[0]); ++i) {
        if (common_services[i].port == port)
            return common_services[i].service;
    }
    return NULL;
}

static int justified_high_ports[] = {8080, 8443, 8888};
static int is_justified_high_port(int port) {
    for (size_t i = 0; i < sizeof(justified_high_ports)/sizeof(justified_high_ports[0]); ++i)
        if (justified_high_ports[i] == port) return 1;
    return 0;
}

void scan_ports_tcp(int start_port, int end_port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Escaneando puertos TCP del %d al %d en localhost...\n", start_port, end_port);

    for (int port = start_port; port <= end_port; ++port) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        addr.sin_port = htons(port);

        int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
        if (result == 0) {
            const char *service = get_service_name(port);
            if (service) {
                printf("Puerto %d ABIERTO (%s)\n", port, service);
            } else if (port > 1024 && !is_justified_high_port(port)) {
                printf("Puerto %d ABIERTO (potencialmente comprometido, alto y desconocido)\n", port);
            } else {
                printf("Puerto %d ABIERTO (servicio desconocido)\n", port);
            }
        }
        close(sock);
    }
}