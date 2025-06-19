#include <gtk/gtk.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/gui.h"

// --- Gestión del proceso controller ---
static pid_t controller_pid = -1;

// Lanza el proceso controller (si no está corriendo)
int start_controller_process() {
    if (controller_pid > 0) {
        // Ya está corriendo
        return 0;
    }
    pid_t pid = fork();
    if (pid == 0) {
        // Proceso hijo: ejecuta el controller
        execl("./great_throne_room/throne_room_controller", "throne_room_controller", NULL);
        // Si execl falla:
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        controller_pid = pid;
        return 1;
    } else {
        perror("fork");
        return -1;
    }
}

// Detiene el proceso controller y espera a que termine
void stop_controller_process() {
    if (controller_pid > 0) {
        kill(controller_pid, SIGTERM);
        waitpid(controller_pid, NULL, 0);
        controller_pid = -1;
    }
}

// --- Función auxiliar para mostrar el reporte en el TextView ---
static void show_report_in_textview(const char *filename, GtkTextView *textview) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(textview), "No hay reporte disponible.", -1);
        return;
    }
    char buffer[8192];
    size_t n = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[n] = '\0';
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(textview), buffer, -1);
    fclose(f);
}

// --- Callbacks de los botones generales ---
static void on_start_scan_clicked(GtkButton *button, gpointer user_data) {
    if (start_controller_process() == 1) {
        g_print("Escaneo iniciado.\n");
    } else {
        g_print("El escaneo ya estaba en ejecución o hubo un error.\n");
    }
}
static void on_stop_scan_clicked(GtkButton *button, gpointer user_data) {
    stop_controller_process();
    g_print("Escaneo detenido.\n");
}
static void on_console_command_entered(GtkEntry *entry, gpointer user_data) {
    const gchar *command = gtk_entry_get_text(entry);
    g_print("Comando ingresado: %s\n", command);
    gtk_entry_set_text(entry, ""); // Limpiar entrada
    // Aquí puedes procesar el comando y mostrar resultado en el textview si lo deseas
}

// --- GUI principal ---
void run_gui() {
    gtk_init(NULL, NULL);

    // Ventana principal
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gran Salón del Trono");
    gtk_window_maximize(GTK_WINDOW(window));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // VBox principal
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 10);

    // === Sección superior con USB, Procesos y Puertos (solo visualización) ===
    GtkWidget *sections_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_hexpand(sections_hbox, TRUE);
    gtk_widget_set_vexpand(sections_hbox, FALSE);

    // Función local para crear cada sección (sin botón)
    GtkWidget* create_section(const gchar *title, GtkWidget **text_view_out) {
        GtkWidget *frame = gtk_frame_new(title);
        gtk_widget_set_hexpand(frame, TRUE);
        gtk_widget_set_vexpand(frame, TRUE);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        GtkWidget *text = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
        gtk_widget_set_hexpand(text, TRUE);
        gtk_widget_set_vexpand(text, TRUE);

        gtk_box_pack_start(GTK_BOX(vbox), text, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        *text_view_out = text;
        return frame;
    }

    GtkWidget *usb_text;
    GtkWidget *proc_text;
    GtkWidget *ports_text;

    GtkWidget *usb_frame = create_section("Dispositivos USB", &usb_text);
    GtkWidget *proc_frame = create_section("Procesos Monitoreados", &proc_text);
    GtkWidget *ports_frame = create_section("Puertos Abiertos", &ports_text);

    gtk_box_pack_start(GTK_BOX(sections_hbox), usb_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), proc_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), ports_frame, TRUE, TRUE, 0);

    // === Consola expandida (fila inferior) ===
    GtkWidget *console_frame = gtk_frame_new("Consola Interactiva");
    gtk_widget_set_hexpand(console_frame, TRUE);
    gtk_widget_set_vexpand(console_frame, TRUE);

    GtkWidget *console_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *console_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_text), FALSE);
    gtk_widget_set_hexpand(console_text, TRUE);
    gtk_widget_set_vexpand(console_text, TRUE);

    GtkWidget *console_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(console_entry), "Escribe un comando y presiona Enter...");

    gtk_box_pack_start(GTK_BOX(console_vbox), console_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(console_vbox), console_entry, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(console_frame), console_vbox);

    // === Botones generales ===
    GtkWidget *controls_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    GtkWidget *start_btn = gtk_button_new_with_label("Comenzar escaneo");
    GtkWidget *stop_btn = gtk_button_new_with_label("Parar escaneo");
    gtk_widget_set_hexpand(start_btn, TRUE);
    gtk_widget_set_hexpand(stop_btn, TRUE);
    gtk_box_pack_start(GTK_BOX(controls_hbox), start_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_hbox), stop_btn, TRUE, TRUE, 0);

    // Empaquetar todo en el VBox principal
    GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);

    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(top_box), sections_hbox, TRUE, TRUE, 0);

    gtk_paned_pack1(GTK_PANED(vpaned), top_box, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(vpaned), console_frame, TRUE, FALSE);

    // Ajusta el divisor para que la sección superior ocupe ~65% y la consola ~35%
    // Si la ventana es de 800px de alto, esto deja 520px arriba y 280px abajo.
    gtk_paned_set_position(GTK_PANED(vpaned), 520);

    gtk_box_pack_start(GTK_BOX(main_vbox), vpaned, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), controls_hbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // Conexión de señales
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_scan_clicked), NULL);
    g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_scan_clicked), NULL);
    g_signal_connect(console_entry, "activate", G_CALLBACK(on_console_command_entered), NULL);

    gtk_widget_show_all(window);
    gtk_main();
}