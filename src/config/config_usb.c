// src/config/config_usb.c
#define _POSIX_C_SOURCE 200809L  // Para que strdup() esté disponible
#include "usb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ini.h"  // del parser inih

static int usb_handler(void* user, const char* section, const char* name, const char* value) {
    usb_config_t* cfg = (usb_config_t*)user;
    if (strcmp(section, "usb") == 0) {
        if (strcmp(name, "mount_point") == 0) {
            cfg->mount_point = strdup(value);
        } else if (strcmp(name, "change_threshold") == 0) {
            cfg->change_threshold = atof(value);
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
    // Valores por defecto
    out_cfg->mount_point      = strdup("/mnt/usb");
    out_cfg->change_threshold = 0.1;
    out_cfg->scan_interval    = 5;

    if (ini_parse(path, usb_handler, out_cfg) < 0) {
        fprintf(stderr, "Error: no se pudo abrir %s\n", path);
        return -1;
    }
    return 0;
}
