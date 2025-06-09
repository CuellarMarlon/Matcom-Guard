#include <stdio.h>
#include "ports/scan_ports/active_scanner.h"

int main(void) {
    // Escaneo activo de puertos TCP del 1 al 1024
    scan_ports_tcp(1025, 2048);
    return 0;
}