#include <stdio.h>
#include "ports/scan_ports/active_scanner.h"

int main(void) {
    // Escaneo activo de puertos TCP del 1 al 40000 (incluye 31337)
    scan_ports_tcp(1, 9999);
    return 0;
}

