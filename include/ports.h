#include <gtk/gtk.h>
#ifndef PORTS_H
#define PORTS_H

void scan_ports_tcp(int start_port, int end_port, GtkTextView *view);
void init_previous_states();

#endif