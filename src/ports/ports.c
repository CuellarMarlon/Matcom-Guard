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

// --- Estructuras ---
typedef struct {
    int puerto;
    const char *servicio;
} PuertoServicio;

typedef struct {
    int puerto_inicio;
    int puerto_fin;
} ArgumentosHilo;

// --- Configuración estática ---
static PuertoServicio servicios_comunes[] = {
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

static const char *obtener_nombre_servicio(int puerto) {
    size_t cantidad = sizeof(servicios_comunes) / sizeof(servicios_comunes[0]);
    for (size_t i = 0; i < cantidad; ++i) {
        if (servicios_comunes[i].puerto == puerto)
            return servicios_comunes[i].servicio;
    }
    return NULL;
}

static int puertos_altos_justificados[] = {8080, 8443, 8888};
static int lista_puertos_sospechosos[] = {31337, 4444, 6667};

static int es_puerto_alto_justificado(int puerto) {
    size_t cantidad = sizeof(puertos_altos_justificados) / sizeof(puertos_altos_justificados[0]);
    for (size_t i = 0; i < cantidad; ++i) {
        if (puertos_altos_justificados[i] == puerto)
            return 1;
    }
    return 0;
}

static int es_puerto_sospechoso(int puerto) {
    size_t cantidad = sizeof(lista_puertos_sospechosos) / sizeof(lista_puertos_sospechosos[0]);
    for (size_t i = 0; i < cantidad; ++i) {
        if (lista_puertos_sospechosos[i] == puerto)
            return 1;
    }
    return 0;
}

// --- Estados previos ---
#define PUERTO_INICIO 1
#define PUERTO_FIN 65535
#define TOTAL_PUERTOS (PUERTO_FIN - PUERTO_INICIO + 1)
static int estados_previos[PUERTO_FIN + 1];  // Indexado por puerto

void inicializar_estados_previos() {
    for (int i = PUERTO_INICIO; i <= PUERTO_FIN; i++) {
        estados_previos[i] = -1;  // Sin estado previo
    }
}

// --- Datos para reportar resultados ---
#define MAX_RESULTADOS 70000
int puertos_permitidos[MAX_RESULTADOS];
int puertos_sospechosos[MAX_RESULTADOS];
int puertos_no_permitidos[MAX_RESULTADOS];
int cantidad_permitidos = 0;
int cantidad_sospechosos = 0;
int cantidad_no_permitidos = 0;

// Intervalos de puertos cerrados: pares [inicio, fin]
int intervalos_cerrados[MAX_RESULTADOS][2];
int cantidad_intervalos = 0;

// Mutex para proteger variables globales
pthread_mutex_t mutex_puertos = PTHREAD_MUTEX_INITIALIZER;

// --- Escaneo de un rango de puertos ---
void *escanear_rango_puertos_tcp(void *argumentos) {
    ArgumentosHilo *rango = (ArgumentosHilo *)argumentos;

    struct sockaddr_in direccion;
    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = inet_addr("127.0.0.1");

    for (int puerto = rango->puerto_inicio; puerto <= rango->puerto_fin; ++puerto) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;

        struct timeval timeout = {1, 0};
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        direccion.sin_port = htons(puerto);
        int resultado = connect(sock, (struct sockaddr *)&direccion, sizeof(direccion));
        close(sock);

        int nuevo_estado = (resultado == 0) ? 1 : 0;

        pthread_mutex_lock(&mutex_puertos);
        estados_previos[puerto] = nuevo_estado;
        pthread_mutex_unlock(&mutex_puertos);
    }

    return NULL;
}

// --- Detectar intervalos consecutivos de puertos cerrados ---
void detectar_intervalos_cerrados() {
    cantidad_intervalos = 0;
    int en_intervalo = 0;
    int inicio = -1;

    for (int puerto = PUERTO_INICIO; puerto <= PUERTO_FIN; ++puerto) {
        if (estados_previos[puerto] == 0) { // cerrado
            if (!en_intervalo) {
                inicio = puerto;
                en_intervalo = 1;
            }
        } else {
            if (en_intervalo) {
                intervalos_cerrados[cantidad_intervalos][0] = inicio;
                intervalos_cerrados[cantidad_intervalos][1] = puerto - 1;
                cantidad_intervalos++;
                en_intervalo = 0;
            }
        }
    }
    if (en_intervalo) {
        intervalos_cerrados[cantidad_intervalos][0] = inicio;
        intervalos_cerrados[cantidad_intervalos][1] = PUERTO_FIN;
        cantidad_intervalos++;
    }
}

// --- Clasifica puertos abiertos según criterios ---
void clasificar_puertos_abiertos() {
    cantidad_permitidos = 0;
    cantidad_sospechosos = 0;
    cantidad_no_permitidos = 0;

    for (int puerto = PUERTO_INICIO; puerto <= PUERTO_FIN; puerto++) {
        if (estados_previos[puerto] == 1) { // abierto
            if (es_puerto_sospechoso(puerto)) {
                if (cantidad_sospechosos < MAX_RESULTADOS)
                    puertos_sospechosos[cantidad_sospechosos++] = puerto;
            } else {
                const char *servicio = obtener_nombre_servicio(puerto);
                if (servicio == NULL && puerto > 1024 && !es_puerto_alto_justificado(puerto)) {
                    if (cantidad_no_permitidos < MAX_RESULTADOS)
                        puertos_no_permitidos[cantidad_no_permitidos++] = puerto;
                } else {
                    if (cantidad_permitidos < MAX_RESULTADOS)
                        puertos_permitidos[cantidad_permitidos++] = puerto;
                }
            }
        }
    }
}

// --- Imprime resultados con formato organizado y visual ---
void imprimir_resultados(GtkTextView *vista) {
    char buffer[512];

    agendar_agregar_texto(vista,  "\n=====================================\n");
    agendar_agregar_texto(vista,  " 🔍 Resultados del escaneo de puertos:\n");
    agendar_agregar_texto(vista,  "=====================================\n\n");

    if (cantidad_permitidos > 0) {
        agendar_agregar_texto(vista,  "✅ Puertos permitidos abiertos:\n");
        for (int i = 0; i < cantidad_permitidos; ++i) {
            const char *servicio = obtener_nombre_servicio(puertos_permitidos[i]);
            if (servicio != NULL)
                snprintf(buffer, sizeof(buffer), "   - Puerto %d (%s)\n", puertos_permitidos[i], servicio);
            else
                snprintf(buffer, sizeof(buffer), "   - Puerto %d (Servicio desconocido)\n", puertos_permitidos[i]);
            agendar_agregar_texto(vista,  buffer);
        }
        agendar_agregar_texto(vista,  "\n");
    }

    if (cantidad_sospechosos > 0) {
        agendar_agregar_texto(vista,  "⚠️ Puertos sospechosos abiertos:\n");
        for (int i = 0; i < cantidad_sospechosos; ++i) {
            snprintf(buffer, sizeof(buffer), "   - Puerto %d (Posible amenaza)\n", puertos_sospechosos[i]);
            agendar_agregar_texto(vista,  buffer);
        }
        agendar_agregar_texto(vista,  "\n");
    }

    if (cantidad_no_permitidos > 0) {
        agendar_agregar_texto(vista,  "🚫 Puertos no permitidos abiertos:\n");
        for (int i = 0; i < cantidad_no_permitidos; ++i) {
            snprintf(buffer, sizeof(buffer), "   - Puerto %d (No permitido y sin servicio conocido)\n", puertos_no_permitidos[i]);
            agendar_agregar_texto(vista,  buffer);
        }
        agendar_agregar_texto(vista,  "\n");
    }

    if (cantidad_intervalos > 0) {
        agendar_agregar_texto(vista,  "🔒 Puertos cerrados en intervalos:\n");
        for (int i = 0; i < cantidad_intervalos; ++i) {
            int inicio = intervalos_cerrados[i][0];
            int fin = intervalos_cerrados[i][1];
            if (inicio == fin)
                snprintf(buffer, sizeof(buffer), "   - Puerto %d cerrado\n", inicio);
            else
                snprintf(buffer, sizeof(buffer), "   - Puertos %d - %d cerrados\n", inicio, fin);
            agendar_agregar_texto(vista,  buffer);
        }
        agendar_agregar_texto(vista,  "\n");
    }
}

#define NUM_HILOS 8

void escanear_puertos_tcp(int puerto_inicio, int puerto_fin, GtkTextView *vista) {
    pthread_t hilos[NUM_HILOS];
    ArgumentosHilo argumentos[NUM_HILOS];

    inicializar_estados_previos();

    printf("🔁 Escaneo de puertos iniciado\n");
    printf("Escaneando puertos TCP del %d al %d en localhost con %d hilos...\n", puerto_inicio, puerto_fin, NUM_HILOS);

    int total_puertos = puerto_fin - puerto_inicio + 1;
    int puertos_por_hilo = total_puertos / NUM_HILOS;

    // Crear hilos para escanear rangos
    for (int i = 0; i < NUM_HILOS; ++i) {
        argumentos[i].puerto_inicio = puerto_inicio + i * puertos_por_hilo;
        argumentos[i].puerto_fin = (i == NUM_HILOS - 1) ? puerto_fin : (argumentos[i].puerto_inicio + puertos_por_hilo - 1);
        pthread_create(&hilos[i], NULL, escanear_rango_puertos_tcp, &argumentos[i]);
    }

    // Esperar a que terminen
    for (int i = 0; i < NUM_HILOS; ++i) {
        pthread_join(hilos[i], NULL);
    }

    // Clasificar puertos abiertos
    clasificar_puertos_abiertos();

    // Detectar intervalos cerrados
    detectar_intervalos_cerrados();

    // Imprimir resultados completos
    imprimir_resultados(vista);
}