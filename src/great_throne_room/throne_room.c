#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "usb.h"
#include "great_throne_room/throne_room.h"
#include "processes.h"
#include "ports.h"

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

void controlador_rf2_processes() {
    main_controller(NULL, NULL);
}

int controlador_rf3_ports() {
    init_previous_states();

    while (1) {
        scan_ports_tcp(1, 65535);
        sleep(10);
    }

    return 0;
}

void controller() {
    pid_t pid_usb = fork();
    if (pid_usb == 0) {
        controlador_rf1_usb(); // Proceso hijo para USB
        exit(EXIT_SUCCESS);
    }

    pid_t pid_proc1 = fork();
    if (pid_proc1 == 0) {
        controlador_rf2_processes(); // Otro hijo
        exit(EXIT_SUCCESS);
    }

    pid_t pid_proc2 = fork();
    if (pid_proc2 == 0) {
        controlador_rf3_ports(); // Otro hijo
        exit(EXIT_SUCCESS);
    }

    // Esperar a los 3 hijos
    int status;
    pid_t wpid;
    int count = 0;
    while ((wpid = wait(&status)) > 0 && count < 3) {
        if (WIFEXITED(status)) {
            printf("Proceso hijo %d terminó con código %d\n", wpid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Proceso hijo %d fue terminado por señal %d\n", wpid, WTERMSIG(status));
        }
        count++;
    }
}