// filepath: /home/marlon/University/Second Year/Second Semester/Operative Systems/06. Project/Matcom-Guard/Matcom-Guard/src/ports.c
#include "ports.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_FILE_PATH "config/matcomguard.conf"

// Dividir una lista de enteros separados por coma
static int parse_ports(const char *value, int *ports, int max_ports) {
    int count = 0;
    char temp[256];
    strncpy(temp, value, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char *token = strtok(temp, ",");
    while (token && count < max_ports) {
        ports[count++] = atoi(token);
        token = strtok(NULL, ",");
    }
    return count;
}

// Dividir una lista de strings separados por coma
static int parse_strings(const char *value, char arr[][32], int max_items) {
    int count = 0;
    char temp[256];
    strncpy(temp, value, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char *token = strtok(temp, ",");
    while (token && count < max_items) {
        strncpy(arr[count], token, 31);
        arr[count][31] = '\0';
        count++;
        token = strtok(NULL, ",");
    }
    return count;
}

void read_config_file_example() {
    FILE *file = fopen(CONFIG_FILE_PATH, "r");
    if (!file) {
        perror("No se pudo abrir el archivo de configuración");
        exit(EXIT_FAILURE);
    }

    char line[256];
    PortGuardConfig config = {0}; // Inicializar con todos los campos en cero

    while (fgets(line, sizeof(line), file)) {
        // Eliminar espacios iniciales
        char *ptr = line;
        while (*ptr == ' ' || *ptr == '\t') {
            ptr++;
        }

        // Ignorar comentarios y lineas vacias
        if(*ptr == '#' || *ptr == '\n' || *ptr == '\0')
        {
            continue;
        }

        // Buscar el caracter '=' para separar clave y valor
        char *equal_sign = strchr(ptr, '=');
        if (equal_sign) {
            *equal_sign = '\0'; // Terminar la clave
            char *key = ptr;
            char *value = equal_sign + 1;

            //Eliminar saltos de linea y espacios finales
            key[strcspn(key, "\r\n ")] = '\0';
            value[strcspn(value, "\r\n ")] = '\0';

            // Guardar los valores en la estructura
            if (strcmp(key, "alert_threshold") == 0) {
                config.alert_threshold = atoi(value);
            } else if (strcmp(key, "monitor_interval") == 0) {
                config.monitor_interval = atoi(value);
            } else if (strcmp(key, "log_level") == 0) {
                strncpy(config.log_level, value, sizeof(config.log_level) - 1);
            } else if (strcmp(key, "log_file") == 0) {
                strncpy(config.log_file, value, sizeof(config.log_file) - 1);
            } else if (strcmp(key, "allowed_ports") == 0) {
                config.allowed_ports_count = parse_ports(value, config.allowed_ports, 32);
            } else if (strcmp(key, "trusted_ips") == 0) {
                config.trusted_ips_count = parse_strings(value, config.trusted_ips, 16);
            } else if (strcmp(key, "whitelist_processes") == 0) {
                config.whitelist_count = parse_strings(value, config.whitelist_processes, 16);
            }
        }

    }

    fclose(file);

    // Imprimir los valores cargados
    printf("allowed_ports: ");
    for (int i = 0; i < config.allowed_ports_count; ++i)
        printf("%d ", config.allowed_ports[i]);
    printf("\n");

    printf("trusted_ips: ");
    for (int i = 0; i < config.trusted_ips_count; ++i)
        printf("%s ", config.trusted_ips[i]);
    printf("\n");

    printf("whitelist_processes: ");
    for (int i = 0; i < config.whitelist_count; ++i)
        printf("%s ", config.whitelist_processes[i]);
    printf("\n");

    printf("Umbral alerta: %d\n", config.alert_threshold);
    printf("Intervalo monitoreo: %d\n", config.monitor_interval);
    printf("Nivel log: %s\n", config.log_level);
    printf("Archivo log: %s\n", config.log_file);
}