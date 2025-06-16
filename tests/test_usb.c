// tests/test_usb.c
#define _XOPEN_SOURCE 700
#include "usb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

typedef struct {
    usb_monitor_t *mon;
    int interval;
} thread_args_t;


static int alerts_triggered = 0;

void alert_handler(const char *message, void *user_data) {
    (void)user_data;
    alerts_triggered++;
    printf("🔔 ALERTA DETECTADA: %s\n", message);
}

void create_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(content, f);
    fclose(f);
}

void *monitor_thread(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    while (alerts_triggered < 3) {
        usb_monitor_scan(args->mon);
        sleep(args->interval);
    }
    return NULL;
}


int main() {
    printf("🚀 Iniciando prueba de monitoreo automatizado con intervenciones cronometradas...\n");

    char temp_dir[] = "/tmp/fake_usb_loop_test_XXXXXX";
    char *usb_dir = mkdtemp(temp_dir);
    assert(usb_dir);

    // Crear archivo inicial
    char path1[256], path2[256], path_new[256];
    snprintf(path1, sizeof(path1), "%s/file1.txt", usb_dir);
    snprintf(path2, sizeof(path2), "%s/file2.txt", usb_dir);
    snprintf(path_new, sizeof(path_new), "%s/nuevo.txt", usb_dir);
    create_file(path1, "contenido 1");
    create_file(path2, "contenido 2");

    // Configurar monitor
    usb_config_t cfg = {
        .mount_point = usb_dir,
        .change_threshold = 0.05,  // 5% para asegurar alerta
        .scan_interval = 5         // cada 5 segundos
    };

    usb_monitor_t *mon = usb_monitor_create(&cfg);
    usb_monitor_set_callback(mon, alert_handler, NULL);

    // Lanzar hilo de escaneo
    pthread_t thread_id;
    thread_args_t args = { .mon = mon, .interval = cfg.scan_interval };
    pthread_create(&thread_id, NULL, monitor_thread, &args);
    
    sleep(3);  // Esperar antes de intervenir
    printf("📝 [T+3s] Creando archivo nuevo...\n");
    create_file(path_new, "archivo nuevo");

    sleep(4);  // T+7s
    printf("✏️  [T+7s] Modificando archivo1...\n");
    create_file(path1, "contenido modificado");

    sleep(5);  // T+12s
    printf("🗑️  [T+12s] Eliminando archivo2...\n");
    unlink(path2);

    pthread_join(thread_id, NULL);

    // Finalización
    usb_monitor_destroy(mon);
    unlink(path1);
    unlink(path_new);
    rmdir(usb_dir);

    assert(alerts_triggered >= 3 && "Se esperaban al menos 3 alertas (nuevo, modificado, eliminado)");
    printf("\n✅ Prueba automática de monitoreo en bucle completada con éxito.\n");
    return 0;
}