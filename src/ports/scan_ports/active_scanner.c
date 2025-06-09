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
    {22, "SSH"}, {80, "HTTP"}, {443, "HTTPS"}, {21, "FTP"}, {25, "SMTP"},
    {53, "DNS"}, {110, "POP3"}, {143, "IMAP"}, {3306, "MySQL"}, {8080, "HTTP-alt"},
    // ... puedes agregar más
};

static const char* get_service_name(int port) {
    for (size_t i = 0; i < sizeof(common_services)/sizeof(common_services[0]); ++i) {
        if (common_services[i].port == port)
            return common_services[i].service;
    }
    return NULL;
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
            if (service)
                printf("Puerto %d ABIERTO (%s)\n", port, service);
            else
                printf("Puerto %d ABIERTO (servicio desconocido)\n", port);
        }
        close(sock);
    }
}