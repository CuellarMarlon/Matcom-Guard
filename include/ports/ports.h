// filepath: /home/marlon/University/Second Year/Second Semester/Operative Systems/06. Project/Matcom-Guard/Matcom-Guard/include/ports.h
#ifndef PORTS_H
#define PORTS_H

// Estructura de configuracion para defensa de puertos
typedef struct {
    int allowed_ports[32];
    int allowed_ports_count;
    char trusted_ips[16][32];
    int trusted_ips_count;
    int alert_threshold;
    int monitor_interval;
    char whitelist_processes[16][32];
    int whitelist_count;
    char log_level[16];
    char log_file[128];
} PortGuardConfig;

void read_config_file_example();

#endif // PORTS_H