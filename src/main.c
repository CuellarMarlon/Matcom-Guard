#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ports/scan_ports/active_scanner.h"

int main(void) {
    printf("Iniciando escaneo continuo en consola, mostrando solo cambios...\n\n");

    init_previous_states();  // Inicializa el almacenamiento de estados

    while (1) {
        system("clear");  // Limpia la pantalla en cada ciclo para actualizar la información
        printf("===  Monitoreando cambios en puertos 0-65535  ===\n");
        scan_ports_tcp(0, 65535);
        sleep(10);  // Espera 10 segundos antes del próximo ciclo
    }

    return 0;
}
