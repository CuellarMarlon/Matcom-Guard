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

volatile int running = 1;

static pid_t pid_usb = -1, pid_proc1 = -1, pid_proc2 = -1;

// Manejador de señales
void handle_termination(int sig) {
    (void)sig;
    printf("\n🚨 Señal recibida. Finalizando...\n");
    running = 0;

    if (pid_usb > 0) kill(pid_usb, SIGTERM);
    if (pid_proc1 > 0) kill(pid_proc1, SIGTERM);
    if (pid_proc2 > 0) kill(pid_proc2, SIGTERM);

    sleep(1);
    exit(0);
}

void detener_controller_desde_gui() {
    handle_termination(0);
}

// ===================== FUNCIONES THREAD-SAFE PARA GTK =====================

typedef struct {
    GtkTextView *view;
    char *text;
} IdleUpdateData;

gboolean idle_append_text(gpointer user_data) {
    IdleUpdateData *d = (IdleUpdateData*)user_data;
    append_text_to_view(d->view, d->text);
    g_free(d->text);
    g_free(d);
    return FALSE;
}

void schedule_append_text(GtkTextView *view, const char *msg) {
    if (!view || !msg) return;
    IdleUpdateData *d = g_new(IdleUpdateData, 1);
    d->view = view;
    d->text = g_strdup(msg);
    g_idle_add(idle_append_text, d);
}

// ===================== RF1: MONITOREO USB =====================

void alert_handler(const char *msg, void *user_data) {
    GuiContext *ctx = (GuiContext *)user_data;

    printf("🔔 [alert_handler] %s\n", msg);
    if (!ctx || !ctx->usb_textview) return;

    schedule_append_text(ctx->usb_textview, msg);
    schedule_append_text(ctx->usb_textview, "\n");
}

void controlador_rf1_usb(GuiContext* ctx) {
    printf("🚀 [controlador_rf1_usb] Iniciando controlador USB\n");

    usb_config_t cfg;
    if (load_usb_config("config/matcomguard.conf", &cfg) != 0) {
        schedule_append_text(ctx->usb_textview, "❌ No se pudo cargar la configuración USB.\n");
        return;
    }

    usb_monitor_t *mon = usb_monitor_create(&cfg);
    usb_monitor_set_callback(mon, alert_handler, ctx);

    schedule_append_text(ctx->usb_textview, "📦 Escaneo inicial...\n");
    usb_monitor_scan(mon);

    while (running) {
        printf("🔁 [controlador_rf1_usb] Escaneo USB...\n");
        sleep(cfg.scan_interval);
        usb_monitor_scan(mon);
    }

    usb_monitor_destroy(mon);
    schedule_append_text(ctx->usb_textview, "🛑 USB Monitor finalizado.\n");
}


// ===================== RF2: MONITOREO DE PROCESOS =====================

void controlador_rf2_processes() {
    printf("🚀 [RF2] Iniciando monitoreo de procesos\n");

    main_controller(NULL, NULL);

    while (running) {
        sleep(1);
    }

    printf("🛑 RF2 terminado\n");
}

// ===================== RF3: ESCANEO DE PUERTOS =====================

int controlador_rf3_ports() {
    printf("🚀 [RF3] Iniciando escaneo de puertos\n");

    init_previous_states();

    while (running) {
        scan_ports_tcp(1, 65535);
        sleep(10);
    }

    printf("🛑 RF3 terminado\n");
    return 0;
}

// ===================== FUNCIÓN PRINCIPAL CONTROLLER =====================

void controller(GuiContext *ctx) {
    // signal(SIGINT, handle_termination);
    // signal(SIGTERM, handle_termination);

    // pid_usb = fork();
    // if (pid_usb == 0) {
    //     controlador_rf1_usb(ctx);
    //     exit(EXIT_SUCCESS);
    // }

    // pid_proc1 = fork();
    // if (pid_proc1 == 0) {
    //     controlador_rf2_processes();
    //     exit(EXIT_SUCCESS);
    // }

    // pid_proc2 = fork();
    // if (pid_proc2 == 0) {
    //     controlador_rf3_ports();
    //     exit(EXIT_SUCCESS);
    // }

    // // Proceso padre espera a los hijos
    // int status;
    // pid_t wpid;
    // int count = 0;
    // while ((wpid = wait(&status)) > 0 && count < 3) {
    //     if (WIFEXITED(status)) {
    //         printf("✔️ Proceso hijo %d terminó con código %d\n", wpid, WEXITSTATUS(status));
    //     } else if (WIFSIGNALED(status)) {
    //         printf("❌ Proceso hijo %d fue terminado por señal %d\n", wpid, WTERMSIG(status));
    //     }
    //     count++;
    // }

    // printf("✅ Todos los procesos hijos han finalizado.\n");
}

