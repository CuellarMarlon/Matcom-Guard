// src/config/config_usb.c
#define _POSIX_C_SOURCE 200809L  // Para que strdup() esté disponible
#include "usb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>    // ⬅️ Para cambiar temporalmente el locale
#include "ini.h"       // del parser inih

static int usb_handler(void* user, const char* section, const char* name, const char* value) {
    usb_config_t* cfg = (usb_config_t*)user;

    if (strcmp(section, "usb") == 0) {
        if (strcmp(name, "mount_point") == 0) {
            cfg->mount_point = strdup(value);
        } else if (strcmp(name, "change_threshold") == 0) {
            // ⚠️ Forzar temporalmente locale "C" para interpretar punto decimal
            char* old_locale = setlocale(LC_NUMERIC, NULL);
            old_locale = strdup(old_locale); // Copiar para restaurar luego

            setlocale(LC_NUMERIC, "C");
            cfg->change_threshold = strtod(value, NULL);
            setlocale(LC_NUMERIC, old_locale);

            free(old_locale);

            printf("📥 Umbral leído como texto: '%s'\n", value);
            printf("📏 Umbral convertido a float: %.5f\n", cfg->change_threshold);
        } else if (strcmp(name, "scan_interval") == 0) {
            cfg->scan_interval = atoi(value);
        } else {
            return 0;  // desconocido
        }
        return 1;
    }
    return 0;  // no nos interesa otra sección
}

int load_usb_config(const char* path, usb_config_t* out_cfg) {
    // ✅ Valores por defecto en caso de que falle la lectura
    out_cfg->mount_point      = strdup("/mnt/usb");
    out_cfg->change_threshold = 0.1f;
    out_cfg->scan_interval    = 5;

    printf("🔍 Cargando configuración desde: %s\n", path);
    if (ini_parse(path, usb_handler, out_cfg) < 0) {
        fprintf(stderr, "❌ Error: no se pudo abrir %s\n", path);
        return -1;
    }

    printf("✅ Configuración cargada:\n");
    printf("   - mount_point: %s\n", out_cfg->mount_point);
    printf("   - scan_interval: %d\n", out_cfg->scan_interval);
    printf("   - change_threshold: %.5f\n", out_cfg->change_threshold);

    return 0;
}
