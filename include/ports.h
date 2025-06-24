#ifndef PORTS_H
#define PORTS_H

#include <gtk/gtk.h>

void escanear_puertos_tcp(int puerto_inicio, int puerto_fin, GtkTextView *vista);
void inicializar_estados_previos();

#endif