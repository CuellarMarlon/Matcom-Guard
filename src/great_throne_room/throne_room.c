#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "usb.h"
#include "great_throne_room/throne_room.h"

static volatile int running = 1;

void alert_handler(const char *msg, void *user_data) {
    (void)user_data;
    printf("🔔 ALERTA: %s\n", msg);
}

void controlador_rf1_usb() {
    usb_config_t cfg;
    if (load_usb_config("config/matcomguard.conf", &cfg) != 0) {
        fprintf(stderr, "❌ No se pudo cargar la configuración USB.\n");
        return;
    }

    usb_monitor_t *mon = usb_monitor_create(&cfg);
    usb_monitor_set_callback(mon, alert_handler, NULL);

    printf("📦 Escaneo inicial...\n");
    usb_monitor_scan(mon);

    while (running) {
        sleep(cfg.scan_interval);
        usb_monitor_scan(mon);
    }

    usb_monitor_destroy(mon);
}

void controller() {
    pid_t pid_usb = fork();
    if (pid_usb == 0) {
        controlador_rf1_usb(); // Proceso hijo para USB
        exit(EXIT_SUCCESS);
    }

    // Cuando tengas más módulos, repites este patrón
    // pid_t pid_proc = fork();
    // if (pid_proc == 0) {
    //     controlador_procesos(); // Por ejemplo
    //     exit(EXIT_SUCCESS);
    // }

    // El proceso padre puede esperar (o no) según el diseño
    int status;
    waitpid(pid_usb, &status, 0);
}



