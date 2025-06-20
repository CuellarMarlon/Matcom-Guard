#include "ports.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>
#include "gui/gui.h"

typedef struct {
    int port;
    const char *service;
} PortService;

typedef struct {
    int start_port;
    int end_port;
} ThreadArgs;

// Configuración estática
static PortService common_services[] = {
    {21, "FTP"}, {22, "SSH"}, {23, "Telnet"}, {25, "SMTP"}, {53, "DNS"},
    {67, "DHCP"}, {68, "DHCP"}, {80, "HTTP"}, {110, "POP3"}, {111, "RPCbind"},
    {123, "NTP"}, {135, "MS RPC"}, {139, "NetBIOS"}, {143, "IMAP"}, {161, "SNMP"},
    {179, "BGP"}, {389, "LDAP"}, {443, "HTTPS"}, {445, "SMB"}, {465, "SMTPS"},
    {514, "Syslog"}, {515, "LPD"}, {587, "Submission"},
    {631, "IPP/CUPS"}, {993, "IMAPS"}, {995, "POP3S"}, {1433, "MSSQL"},
    {1521, "Oracle"}, {2049, "NFS"}, {3306, "MySQL"}, {3389, "RDP"},
    {5432, "PostgreSQL"}, {5900, "VNC"},
    {8080, "HTTP-alt"}, {8443, "HTTPS-alt"}, {8888, "Alternate HTTP"},
    {6667, "IRC"}, {31337, "Back Orifice/Elite"}, {4444, "Metasploit/Backdoor"}
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
static int suspicious_ports[] = {31337, 4444, 6667};

static int is_justified_high_port(int port) {
    size_t len = sizeof(justified_high_ports) / sizeof(justified_high_ports[0]);
    for (size_t i = 0; i < len; ++i) {
        if (justified_high_ports[i] == port)
            return 1;
    }
    return 0;
}

static int is_suspicious_port(int port) {
    size_t len = sizeof(suspicious_ports) / sizeof(suspicious_ports[0]);
    for (size_t i = 0; i < len; ++i) {
        if (suspicious_ports[i] == port)
            return 1;
    }
    return 0;
}

// Estados previos para todo el rango
#define START_PORT 1
#define END_PORT 65535
#define NUM_PORTS (END_PORT - START_PORT + 1)
static int previous_states[END_PORT + 1];  // Indexed by port

void init_previous_states() {
    for (int i = START_PORT; i <= END_PORT; i++) {
        previous_states[i] = -1;  // Sin estado previo
    }
}

// --- Datos para reportar resultados ---
#define MAX_RESULTS 70000
int open_permitidos[MAX_RESULTS];
int open_sospechosos[MAX_RESULTS];
int open_no_permitidos[MAX_RESULTS];
int count_permitidos = 0;
int count_sospechosos = 0;
int count_no_permitidos = 0;

// Intervalos de puertos cerrados: pares [inicio, fin]
int closed_intervals[MAX_RESULTS][2];
int count_intervals = 0;

// Mutex para proteger variables globales
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *scan_ports_range_tcp(void *args) {
    ThreadArgs *range = (ThreadArgs *)args;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    for (int port = range->start_port; port <= range->end_port; ++port) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;

        struct timeval timeout = {1, 0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        addr.sin_port = htons(port);
        int result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        close(sock);

        int new_status = (result == 0) ? 1 : 0;

        pthread_mutex_lock(&mutex);
        // Guardamos siempre el estado actual, no filtramos por cambios
        previous_states[port] = new_status;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

// Detectar intervalos consecutivos de puertos cerrados
void detectar_intervalos_cerrados() {
    count_intervals = 0;
    int in_interval = 0;
    int start = -1;

for (int port = START_PORT; port <= END_PORT; ++port) {
        if (previous_states[port] == 0) { // cerrado
            if (!in_interval) {
                start = port;
                in_interval = 1;
            }
        } else {
            if (in_interval) {
                closed_intervals[count_intervals][0] = start;
                closed_intervals[count_intervals][1] = port - 1;
                count_intervals++;
                in_interval = 0;
            }
        }
    }
    if (in_interval) {
        closed_intervals[count_intervals][0] = start;
        closed_intervals[count_intervals][1] = END_PORT;
        count_intervals++;
    }
}

// Clasifica puertos abiertos según criterios, llena arreglos
void clasificar_puertos_abiertos() {
    count_permitidos = 0;
    count_sospechosos = 0;
    count_no_permitidos = 0;

    for (int port = START_PORT; port <= END_PORT; port++) {
        if (previous_states[port] == 1) { // abierto
            if (is_suspicious_port(port)) {
                if (count_sospechosos < MAX_RESULTS)
                    open_sospechosos[count_sospechosos++] = port;
            } else {
                const char *service = get_service_name(port);
                if (service == NULL && port > 1024 && !is_justified_high_port(port)) {
                    if (count_no_permitidos < MAX_RESULTS)
                        open_no_permitidos[count_no_permitidos++] = port;
                } else {
                    if (count_permitidos < MAX_RESULTS)
                        open_permitidos[count_permitidos++] = port;
                }
            }
        }
    }
}

// Imprime resultados con formato organizado y visual
void imprimir_resultados(GtkTextView *view) {
    char buffer[512];

    append_text_to_view(view, "\n=====================================\n");
    append_text_to_view(view, " 🔍 Resultados del escaneo de puertos:\n");
    append_text_to_view(view, "=====================================\n\n");

    if (count_permitidos > 0) {
        append_text_to_view(view, "✅ Puertos permitidos abiertos:\n");
        for (int i = 0; i < count_permitidos; ++i) {
            const char *service = get_service_name(open_permitidos[i]);
            if (service != NULL)
                snprintf(buffer, sizeof(buffer), "   - Puerto %d (%s)\n", open_permitidos[i], service);
            else
                snprintf(buffer, sizeof(buffer), "   - Puerto %d (Servicio desconocido)\n", open_permitidos[i]);
            append_text_to_view(view, buffer);
        }
        append_text_to_view(view, "\n");
    }

    if (count_sospechosos > 0) {
        append_text_to_view(view, "⚠️ Puertos sospechosos abiertos:\n");
        for (int i = 0; i < count_sospechosos; ++i) {
            snprintf(buffer, sizeof(buffer), "   - Puerto %d (Posible amenaza)\n", open_sospechosos[i]);
            append_text_to_view(view, buffer);
        }
        append_text_to_view(view, "\n");
    }

    if (count_no_permitidos > 0) {
        append_text_to_view(view, "🚫 Puertos no permitidos abiertos:\n");
        for (int i = 0; i < count_no_permitidos; ++i) {
            snprintf(buffer, sizeof(buffer), "   - Puerto %d (No permitido y sin servicio conocido)\n", open_no_permitidos[i]);
            append_text_to_view(view, buffer);
        }
        append_text_to_view(view, "\n");
    }

    if (count_intervals > 0) {
        append_text_to_view(view, "🔒 Puertos cerrados en intervalos:\n");
        for (int i = 0; i < count_intervals; ++i) {
            int start = closed_intervals[i][0];
            int end = closed_intervals[i][1];
            if (start == end)
                snprintf(buffer, sizeof(buffer), "   - Puerto %d cerrado\n", start);
            else
                snprintf(buffer, sizeof(buffer), "   - Puertos %d - %d cerrados\n", start, end);
            append_text_to_view(view, buffer);
        }
        append_text_to_view(view, "\n");
    }
}

#define NUM_THREADS 8

void scan_ports_tcp(int start_port, int end_port, GtkTextView *view) {
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];

init_previous_states();

    printf("🔁 Escaneo de puertos iniciado\n");
    printf("Escaneando puertos TCP del %d al %d en localhost con %d hilos...\n", start_port, end_port, NUM_THREADS);

    int total_ports = end_port - start_port + 1;
    int ports_per_thread = total_ports / NUM_THREADS;

    // Crear hilos para escanear rangos
    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].start_port = start_port + i * ports_per_thread;
        args[i].end_port = (i == NUM_THREADS - 1) ? end_port : (args[i].start_port + ports_per_thread - 1);
        pthread_create(&threads[i], NULL, scan_ports_range_tcp, &args[i]);
    }

    // Esperar a que terminen
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Clasificar puertos abiertos
    clasificar_puertos_abiertos();

    // Detectar intervalos cerrados
    detectar_intervalos_cerrados();

    // Imprimir resultados completos
    imprimir_resultados(view);
}