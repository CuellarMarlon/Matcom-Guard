// matcom_guard_gui.c
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include<signal.h>
#include <time.h>
#include <processes.h>

#define MAX_PROC 32768
#define MAX_WHITELIST 32

// Umbrales configurables
int UMBRAL_CPU = 70;      // %
int UMBRAL_RAM = 50;      // %
int UMBRAL_TIEMPO = 1;   // segundos
const char* WHITELIST[MAX_WHITELIST] = { "Xorg","gnome-shell" ,"matcomguard", "gcc" };
int WHITELIST_LEN = 3;

ProcesoInfo procesos_sospechosos[MAX_PROC];
pthread_mutex_t mutex_procs = PTHREAD_MUTEX_INITIALIZER;

// GTK widgets globales
GtkWidget* treeview;
GtkListStore* list_store;

// Funciones auxiliares (igual que antes)
int es_numero(const char* str) {
    for (int i = 0; str[i]; i++) if (!isdigit(str[i])) return 0;
    return 1;
}

int en_whitelist(const char* nombre) {
    for (int i = 0; i < WHITELIST_LEN; ++i)
        if (strcmp(nombre, WHITELIST[i]) == 0) return 1;
    return 0;
}

void obtener_nombre_proceso(pid_t pid, char* nombre) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE* f = fopen(path, "r");
    if (f) {
        fgets(nombre, 256, f);
        nombre[strcspn(nombre, "\n")] = 0;
        fclose(f);
    }
    else {
        strcpy(nombre, "desconocido");
    }
}

float obtener_uso_ram(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char linea[256];
    float rss = 0;
    while (fgets(linea, sizeof(linea), f)) {
        if (strncmp(linea, "VmRSS:", 6) == 0) {
            sscanf(linea, "VmRSS: %f", &rss);
            break;
        }
    }
    fclose(f);

    FILE* mem = fopen("/proc/meminfo", "r");
    float total = 0;
    if (!mem) return 0;
    while (fgets(linea, sizeof(linea), mem)) {
        if (strncmp(linea, "MemTotal:", 9) == 0) {
            sscanf(linea, "MemTotal: %f", &total);
            break;
        }
    }
    fclose(mem);

    if (total == 0) return 0;
    return (rss / total) * 100.0;
}

// Variables para cálculo real de CPU
typedef struct {
    unsigned long utime;
    unsigned long stime;
} CpuTimes;

int obtener_cpu_times(pid_t pid, CpuTimes* times) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    // stat tiene muchos campos, utime es 14, stime es 15
    // Leer hasta el campo 15
    unsigned long utime = 0, stime = 0;
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    fclose(f);

    // strtok para obtener campos
    char* tok = strtok(buffer, " ");
    int i = 1;
    while (tok && i <= 15) {
        if (i == 14) utime = strtoul(tok, NULL, 10);
        if (i == 15) stime = strtoul(tok, NULL, 10);
        tok = strtok(NULL, " ");
        i++;
    }
    if (utime == 0 && stime == 0) return 0;

    times->utime = utime;
    times->stime = stime;
    return 1;
}

// Variables globales para CPU
#define CPU_TICKS_PER_SEC sysconf(_SC_CLK_TCK)

typedef struct {
    pid_t pid;
    CpuTimes prev_times;
    int initialized;
} CpuInfo;

CpuInfo cpu_info[MAX_PROC];

// Función para calcular % CPU en intervalo (1s)
float calcular_uso_cpu(pid_t pid) {
    CpuTimes current;
    if (!obtener_cpu_times(pid, &current)) return 0;

    CpuInfo* info = &cpu_info[pid];

    if (!info->initialized) {
        info->prev_times = current;
        info->pid = pid;
        info->initialized = 1;
        return 0; // no hay intervalo para calcular todavía
    }

    unsigned long total_prev = info->prev_times.utime + info->prev_times.stime;
    unsigned long total_curr = current.utime + current.stime;

    unsigned long diff = total_curr - total_prev;

    // Actualizamos para el próximo cálculo
    info->prev_times = current;

    // % CPU = (ticks usados en 1 segundo) / ticks por segundo * 100
    float uso = ((float)diff / CPU_TICKS_PER_SEC) * 100.0f;

    // límite a 100%
    if (uso > 100.0f) uso = 100.0f;

    return uso;
}

// Añadir o actualizar proceso sospechoso
void actualizar_proceso_sospechoso(pid_t pid, const char* nombre, float uso_cpu, float uso_ram) {
    pthread_mutex_lock(&mutex_procs);

    ProcesoInfo* p = &procesos_sospechosos[pid];
    if (!p->activo) {
        p->pid = pid;
        strncpy(p->nombre, nombre, sizeof(p->nombre) - 1);
        p->nombre[sizeof(p->nombre) - 1] = 0;
        p->tiempo_sospechoso = 0;
        p->activo = 1;
    }
    p->uso_cpu = uso_cpu;
    p->uso_ram = uso_ram;
    p->tiempo_sospechoso++;

    pthread_mutex_unlock(&mutex_procs);
}

// Resetear proceso sospechoso (cuando vuelve a estar normal)
void resetear_proceso(pid_t pid) {
    pthread_mutex_lock(&mutex_procs);
    procesos_sospechosos[pid].activo = 0;
    pthread_mutex_unlock(&mutex_procs);
}

// Actualizar lista GTK con procesos sospechosos
gboolean actualizar_lista_gui(gpointer data) {
    pthread_mutex_lock(&mutex_procs);
    gtk_list_store_clear(list_store);

    for (int i = 0; i < MAX_PROC; i++) {
        ProcesoInfo* p = &procesos_sospechosos[i];
        if (p->activo && p->tiempo_sospechoso >= UMBRAL_TIEMPO) {
            char pid_str[16];
            snprintf(pid_str, sizeof(pid_str), "%d", p->pid);
            char cpu_str[16];
            snprintf(cpu_str, sizeof(cpu_str), "%.2f%%", p->uso_cpu);
            char ram_str[16];
            snprintf(ram_str, sizeof(ram_str), "%.2f%%", p->uso_ram);
            char tiempo_str[16];
            snprintf(tiempo_str, sizeof(tiempo_str), "%ds", p->tiempo_sospechoso*10);

            gtk_list_store_insert_with_values(list_store, NULL, -1,
                0, pid_str,
                1, p->nombre,
                2, cpu_str,
                3, ram_str,
                4, tiempo_str,
                -1);
        }
    }

    pthread_mutex_unlock(&mutex_procs);
    return TRUE; // Para que se llame periódicamente
}

// Monitoreo de procesos en hilo separado
void* hilo_monitoreo(void* arg) {
    while (1) {
        printf("🧠 Hilo monitoreo de procesos activo...\n");
        DIR* proc = opendir("/proc");
        struct dirent* entry;
        if (!proc) {
            perror("No se pudo abrir /proc");
            sleep(10);
            continue;
        }

        while ((entry = readdir(proc)) != NULL) {
            if (!es_numero(entry->d_name)) continue;
            pid_t pid = atoi(entry->d_name);

            char nombre[256];
            obtener_nombre_proceso(pid, nombre);

            if (en_whitelist(nombre)) {
                resetear_proceso(pid);
                continue;
            }

            float uso_cpu = calcular_uso_cpu(pid);
            float uso_ram = obtener_uso_ram(pid);

            if (uso_cpu > UMBRAL_CPU || uso_ram > UMBRAL_RAM) {
                printf("⚠️ Proceso sospechoso: %s (PID %d) CPU: %.1f RAM: %.1f\n", nombre, pid, uso_cpu, uso_ram);
                actualizar_proceso_sospechoso(pid, nombre, uso_cpu, uso_ram);
            }
            else {
                resetear_proceso(pid);
            }
        }

        closedir(proc);
        sleep(10);
    }
    return NULL;
}

// Crear columnas del GtkTreeView
void crear_columnas(GtkWidget* treeview) {
    GtkCellRenderer* renderer;

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
        -1, "PID", renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
        -1, "Nombre", renderer, "text", 1, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
        -1, "% CPU", renderer, "text", 2, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
        -1, "% RAM", renderer, "text", 3, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
        -1, "Tiempo sospechoso (s)", renderer, "text", 4, NULL);
}

int main_controller(int argc, char* argv[]) {
    // Solo lanza el hilo de monitoreo, no crea ninguna interfaz gráfica
    pthread_t tid;
    pthread_create(&tid, NULL, hilo_monitoreo, NULL);

    return 0;
}
//-----------------------------------------------------------