#ifndef MATCOM_GUARD_GUI_H
#define MATCOM_GUARD_GUI_H

#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/types.h>
#include <glib-2.0/glib/gtypes.h>
#include <gtk-3.0/gtk/gtktypes.h>

// --- Constantes ---
#define MAX_PROC 32768
#define MAX_WHITELIST 32

// --- Estructura que representa un proceso sospechoso ---
typedef struct {
    pid_t pid;
    char nombre[256];
    float uso_cpu;
    float uso_ram;
    int tiempo_sospechoso;
    int activo;
} ProcesoInfo;

// --- Variables globales compartidas ---
extern ProcesoInfo procesos_sospechosos[MAX_PROC];
extern pthread_mutex_t mutex_procs;

// --- Función principal del controlador lógico (sin GUI ahora) ---
int main_controller(int argc, char* argv[]);

// --- Funciones utilitarias ---
int es_numero(const char* str);
int en_whitelist(const char* nombre);
void obtener_nombre_proceso(pid_t pid, char* nombre);
float obtener_uso_ram(pid_t pid);
float calcular_uso_cpu(pid_t pid);

void actualizar_proceso_sospechoso(pid_t pid, const char* nombre, float uso_cpu, float uso_ram);
void resetear_proceso(pid_t pid);

// --- Funciones gráficas (para integrarse con GUI principal) ---
gboolean actualizar_lista_gui(gpointer data);
void crear_columnas(GtkWidget* treeview);
// extern GtkWidget* treeview;          // <- solo si accedes desde gui.c
// extern GtkListStore* list_store;     // <- solo si accedes desde gui.c

#endif // MATCOM_GUARD_GUI_H

