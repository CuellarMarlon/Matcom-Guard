// src/main.c
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "usb.h"

static volatile int running = 1;

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
    printf("\n⛔ Finalizando monitoreo USB...\n");
}

void alert_handler(const char *message, void *user_data) {
    (void)user_data;
    printf("🔔 ALERTA: %s\n", message);
}

int main() {
    // Capturar Ctrl+C
    signal(SIGINT, handle_sigint);

    usb_config_t cfg;
    if (load_usb_config("config/matcomguard.conf", &cfg) != 0) {
        fprintf(stderr, "❌ Error al cargar configuración USB.\n");
        return 1;
    }

    printf("🧪 Iniciando monitoreo USB en: %s (umbral %.0f%%, intervalo %d s)\n",
           cfg.mount_point, cfg.change_threshold * 100, cfg.scan_interval);

    usb_monitor_t *mon = usb_monitor_create(&cfg);
    usb_monitor_set_callback(mon, alert_handler, NULL);

    // Escaneo inicial
    usb_monitor_scan(mon);

    // Bucle de escaneo
    while (running) {
        sleep(cfg.scan_interval);
        usb_monitor_scan(mon);
    }

    usb_monitor_destroy(mon);
    printf("✅ Monitoreo finalizado correctamente.\n");
    return 0;
}
