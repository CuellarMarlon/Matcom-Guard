#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "usb.h"
#include "great_throne_room/throne_room.h"
#include "processes.h"
#include "ports.h"
#include "gui/gui.h"

volatile int ejecutando = 1; // <--- ÚNICA definición real

static pid_t pid_usb = -1, pid_proc1 = -1, pid_proc2 = -1;

// Nueva función: solo detiene los procesos hijos, no termina la app
void detener_controlador_desde_gui() {
    ejecutando = 0;

    if (pid_usb > 0) {
        kill(pid_usb, SIGTERM);
        pid_usb = -1;
    }
    if (pid_proc1 > 0) {
        kill(pid_proc1, SIGTERM);
        pid_proc1 = -1;
    }
    if (pid_proc2 > 0) {
        kill(pid_proc2, SIGTERM);
        pid_proc2 = -1;
    }
}

// Manejador de señales: sigue cerrando todo (incluye exit)
void manejador_terminacion(int sig) {
    (void)sig;
    printf("\n🚨 Señal recibida. Finalizando...\n");
    ejecutando = 0;

    if (pid_usb > 0) kill(pid_usb, SIGTERM);
    if (pid_proc1 > 0) kill(pid_proc1, SIGTERM);
    if (pid_proc2 > 0) kill(pid_proc2, SIGTERM);

    sleep(1);
    exit(0);
}

// ===================== FUNCIONES THREAD-SAFE PARA GTK =====================

typedef struct {
    GtkTextView *vista;
    char *texto;
} DatosActualizacionIdle;

gboolean idle_agregar_texto(gpointer datos_usuario) {
    DatosActualizacionIdle *d = (DatosActualizacionIdle*)datos_usuario;
    append_text_to_view(d->vista, d->texto);
    g_free(d->texto);
    g_free(d);
    return FALSE;
}

void agendar_agregar_texto(GtkTextView *vista, const char *mensaje) {
    if (!vista || !mensaje) return;
    DatosActualizacionIdle *d = g_new(DatosActualizacionIdle, 1);
    d->vista = vista;
    d->texto = g_strdup(mensaje);
    g_idle_add(idle_agregar_texto, d);
}

// ===================== RF1: MONITOREO USB =====================

void manejador_alerta(const char *mensaje, void *datos_usuario) {
    GuiContext *ctx = (GuiContext *)datos_usuario;

    printf("🔔 [manejador_alerta] %s\n", mensaje);
    if (!ctx || !ctx->usb_textview) return;

    agendar_agregar_texto(ctx->usb_textview, mensaje);
    agendar_agregar_texto(ctx->usb_textview, "\n");
}

void controlador_rf1_usb(GuiContext* ctx) {
    printf("🚀 [controlador_rf1_usb] Iniciando controlador USB\n");

    usb_config_t cfg;
    if (load_usb_config("config/matcomguard.conf", &cfg) != 0) {
        agendar_agregar_texto(ctx->usb_textview, "❌ No se pudo cargar la configuración USB.\n");
        return;
    }

    usb_monitor_t *mon = usb_monitor_create(&cfg);
    usb_monitor_set_callback(mon, manejador_alerta, ctx);

    agendar_agregar_texto(ctx->usb_textview, "📦 Escaneo inicial...\n");
    usb_monitor_scan(mon);

    while (ejecutando) {
        printf("🔁 [controlador_rf1_usb] Escaneo USB...\n");
        sleep(cfg.scan_interval);
        usb_monitor_scan(mon);
    }

    usb_monitor_destroy(mon);
    agendar_agregar_texto(ctx->usb_textview, "🛑 USB Monitor finalizado.\n");
}


// ===================== RF2: MONITOREO DE PROCESOS =====================

void controlador_rf2_procesos() {
    printf("🚀 [RF2] Iniciando monitoreo de procesos\n");

    main_controller(0, NULL);

    while (ejecutando) {
        sleep(1);
    }

    printf("🛑 RF2 terminado\n");
}

// ===================== RF3: ESCANEO DE PUERTOS =====================

int controlador_rf3_puertos(GuiContext *ctx) {
    printf("🚀 [RF3] Iniciando escaneo de puertos\n");

    inicializar_estados_previos();

    while (ejecutando) {
        escanear_puertos_tcp(1, 65535, ctx->ports_textview);
        sleep(10);
    }

    agendar_agregar_texto(ctx->ports_textview, "🛑 RF3 finalizado.\n");
    return 0;
}