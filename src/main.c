// main.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ports/scan_ports/active_scanner.h"

void test_legit_port() {
    // Caso: Puerto legítimo (SSH en el puerto 22)
    printf("Prueba: SSH activo en puerto 22 (permitido)...\n");
    system("sudo systemctl start sshd"); // Se asume que SSH está correctamente configurado.
    sleep(1);
    scan_ports_tcp(22, 22);
}

void test_suspicious_port() {
    // Caso: Puerto sospechoso (31337) usando netcat
    printf("\nPrueba: Puerto sospechoso 31337 (no permitido)...\n");
    system("pkill -f 'nc -l 31337'"); // Asegura no tener procesos previos.
    system("nc -l 31337 &");          // Lanza netcat en 31337.
    sleep(1);
    scan_ports_tcp(31337, 31337);
    system("pkill -f 'nc -l 31337'");
}

void test_justified_high_port() {
    // Caso: Puerto alto justificado (8080), que se espera esté permitido.
    printf("\nPrueba: Puerto justificado alto 8080 (permitido, HTTP-alt)...\n");
    system("pkill -f 'python3 -m http.server 8080'");  // Limpia posibles instancias previas.
    system("python3 -m http.server 8080 &");            // Inicia un server HTTP en 8080.
    sleep(1);
    scan_ports_tcp(8080, 8080);
    system("pkill -f 'python3 -m http.server 8080'");
}

void test_closed_port() {
    // Caso: Puerto sin servicio (cerrado)
    printf("\nPrueba: Puerto 9999 cerrado (estado normal)...\n");
    scan_ports_tcp(9999, 9999);
}

void test_reopen_closed_port() {
    // Caso: Puerto 5555 inicialmente cerrado; luego se abre y se cierra, mostrando transición.
    printf("\nPrueba: Puerto 5555 inicialmente cerrado, luego se abre y vuelve a cerrarse...\n");
    
    printf("Fase 1: Verificando que 5555 esté cerrado...\n");
    scan_ports_tcp(5555, 5555);
    sleep(1);
    
    printf("Fase 2: Abriendo puerto 5555 con netcat...\n");
    system("pkill -f 'nc -l 5555'");
    system("nc -l 5555 &");
    sleep(2);
    printf("Verificando que 5555 esté abierto (se debería marcar alerta, ya que no está en la lista de servicios y no es justificado)...\n");
    scan_ports_tcp(5555, 5555);
    sleep(1);
    
    printf("Fase 3: Cerrando puerto 5555...\n");
    system("pkill -f 'nc -l 5555'");
    sleep(1);
    printf("Verificando nuevamente 5555 (debe volver a reportarse como cerrado)...\n");
    scan_ports_tcp(5555, 5555);
}

void test_range_scan() {
    // Caso: Escanear un rango de puertos para ver distintos estados.
    printf("\nPrueba: Escaneo de rango de puertos 20-30...\n");
    scan_ports_tcp(20, 30);
}

void test_looping_changes() {
    // Caso: Puerto 7777 que cambia varias veces (abierto-cerrado, ciclo repetitivo)
    printf("\nPrueba: Puerto 7777 que cambia de estado (open-close-open-close)...\n");
    
    printf("Verificando inicialmente que 7777 esté cerrado...\n");
    scan_ports_tcp(7777, 7777);
    sleep(1);
    
    printf("Abriendo puerto 7777 con netcat...\n");
    system("pkill -f 'nc -l 7777'");
    system("nc -l 7777 &");
    sleep(1);
    scan_ports_tcp(7777, 7777);
    sleep(1);
    
    printf("Cerrando puerto 7777...\n");
    system("pkill -f 'nc -l 7777'");
    sleep(1);
    scan_ports_tcp(7777, 7777);
    sleep(1);
    
    printf("Abriendo puerto 7777 nuevamente con netcat...\n");
    system("nc -l 7777 &");
    sleep(1);
    scan_ports_tcp(7777, 7777);
    system("pkill -f 'nc -l 7777'");
}

int main(void) {
    printf("Iniciando pruebas robustas de escaneo de puertos...\n\n");
    
    test_legit_port();
    sleep(2);
    
    test_suspicious_port();
    sleep(2);
    
    test_justified_high_port();
    sleep(2);
    
    test_closed_port();
    sleep(2);
    
    test_reopen_closed_port();
    sleep(2);
    
    test_range_scan();
    sleep(2);
    
    test_looping_changes();
    sleep(2);
    
    printf("\nTodas las pruebas han finalizado.\n");
    
    return 0;
}
