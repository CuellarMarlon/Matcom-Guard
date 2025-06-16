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
    {123, "NTP"},   {135, "MS RPC"}, {139, "NetBIOS"},{143, "IMAP"},   {161, "SNMP"},
    {179, "BGP"},   {389, "LDAP"},   {443, "HTTPS"}, {445, "SMB"},    {465, "SMTPS"},
    {514, "Syslog"},{515, "LPD"},    {587, "Submission"},
    {631, "IPP/CUPS"},{993, "IMAPS"},{995, "POP3S"}, {1433, "MSSQL"}, {1521, "Oracle"},
    {2049, "NFS"},  {3306, "MySQL"}, {3389, "RDP"},  {5432, "PostgreSQL"},{5900, "VNC"},
    {8080, "HTTP-alt"},{8443, "HTTPS-alt"},{8888, "Alternate HTTP"},
    {6667, "IRC"},  {31337, "Back Orifice/Elite"},{4444, "Metasploit/Backdoor"}
    // Puedes agregar más según necesidades.
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
// Almacenamiento del estado previo para evitar spam en la consola
// =======================================================
#define START_PORT 0
#define END_PORT 65535
#define NUM_PORTS (END_PORT - START_PORT + 1)
static int previous_states[NUM_PORTS];

void init_previous_states() {
    for (int i = 0; i < NUM_PORTS; i++) {
        previous_states[i] = -1;  // -1 indica que aún no hay un estado previo (primera iteración)
    }
}

// =======================================================
// Función de escaneo de puertos
// =======================================================
/*
 * Escanea puertos TCP en el rango [start_port, end_port] en localhost.
 * La lógica es la siguiente:
 *   - Si se logra conectar (puerto abierto):
 *       * Se chequea si está en la lista de puertos sospechosos: se genera "Alerta".
 *       * Si no se encuentra en la lista de servicios comunes y el puerto es alto (>1024)
 *         y no está en la lista de justificación, se genera "Alerta".
 *       * De lo contrario, se genera "Reporte:" indicando que está permitido.
 *   - Si no se logra conectar, se reporta que el puerto está cerrado.
 *
 * Ahora se compara el estado actual del puerto (abierto/cerrado) con el estado
 * del ciclo anterior y se imprime el mensaje únicamente si hubo un cambio.
 */
void scan_ports_tcp(int start_port, int end_port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Escaneando puertos TCP del %d al %d en localhost...\n", start_port, end_port);

    for (int port = start_port; port <= end_port; ++port) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
            continue;

        // Configurar un timeout corto de 1 segundo
        struct timeval timeout = {1, 0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        addr.sin_port = htons(port);
        int result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        close(sock);

        int new_status = (result == 0) ? 1 : 0;
        int index = port - start_port;

        // Solo imprimir si el estado cambió respecto al ciclo anterior
        if (previous_states[index] != new_status) {
            if (result == 0) { // Si se conecta, significa que el puerto está abierto
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
            } else {
                printf("Reporte: \"Puerto %d/tcp cerrado\"\n", port);
            }
            previous_states[index] = new_status;  // Actualizar el estado para la próxima comparación
        }
    }
}
