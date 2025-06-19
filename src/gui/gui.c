#include <gtk/gtk.h>
#include "gui/gui.h"

// Callbacks de ejemplo (puedes conectar la lógica real después)
static void on_scan_usb_clicked(GtkButton *button, gpointer user_data) {
    g_print("Escanear USB...\n");
}
static void on_scan_processes_clicked(GtkButton *button, gpointer user_data) {
    g_print("Escanear procesos...\n");
}
static void on_scan_ports_clicked(GtkButton *button, gpointer user_data) {
    g_print("Escanear puertos...\n");
}
static void on_start_scan_clicked(GtkButton *button, gpointer user_data) {
    g_print("Comenzar escaneo general...\n");
}
static void on_stop_scan_clicked(GtkButton *button, gpointer user_data) {
    g_print("Parar escaneo general...\n");
}
static void on_console_command_entered(GtkEntry *entry, gpointer user_data) {
    const gchar *command = gtk_entry_get_text(entry);
    g_print("Comando ingresado: %s\n", command);
    gtk_entry_set_text(entry, ""); // Limpiar entrada
    // Aquí puedes procesar el comando y mostrar resultado en el textview
}

void run_gui() {
    gtk_init(NULL, NULL);

    // Ventana principal
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gran Salón del Trono");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // VBox principal
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 10);

    // HBox para las tres secciones
    GtkWidget *sections_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    // --- Sección USB ---
    GtkWidget *usb_frame = gtk_frame_new("Dispositivos USB");
    GtkWidget *usb_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *usb_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(usb_text), FALSE);
    gtk_widget_set_size_request(usb_text, 300, 200);
    GtkWidget *usb_scan_btn = gtk_button_new_with_label("Escanear USB");
    gtk_box_pack_start(GTK_BOX(usb_vbox), usb_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(usb_vbox), usb_scan_btn, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(usb_frame), usb_vbox);

    // --- Sección Procesos ---
    GtkWidget *proc_frame = gtk_frame_new("Procesos Monitoreados");
    GtkWidget *proc_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *proc_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(proc_text), FALSE);
    gtk_widget_set_size_request(proc_text, 300, 200);
    GtkWidget *proc_scan_btn = gtk_button_new_with_label("Escanear Procesos");
    gtk_box_pack_start(GTK_BOX(proc_vbox), proc_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(proc_vbox), proc_scan_btn, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(proc_frame), proc_vbox);

    // --- Sección Puertos ---
    GtkWidget *ports_frame = gtk_frame_new("Puertos Abiertos");
    GtkWidget *ports_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *ports_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(ports_text), FALSE);
    gtk_widget_set_size_request(ports_text, 300, 200);
    GtkWidget *ports_scan_btn = gtk_button_new_with_label("Escanear Puertos");
    gtk_box_pack_start(GTK_BOX(ports_vbox), ports_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(ports_vbox), ports_scan_btn, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ports_frame), ports_vbox);

    // Añadir las tres secciones al hbox
    gtk_box_pack_start(GTK_BOX(sections_hbox), usb_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), proc_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), ports_frame, TRUE, TRUE, 0);

    // --- Consola interactiva ---
    GtkWidget *console_frame = gtk_frame_new("Consola Interactiva");
    GtkWidget *console_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *console_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_text), FALSE);
    gtk_widget_set_size_request(console_text, -1, 120);
    GtkWidget *console_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(console_entry), "Escribe un comando y presiona Enter...");
    gtk_box_pack_start(GTK_BOX(console_vbox), console_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(console_vbox), console_entry, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(console_frame), console_vbox);

    // --- Botones de control generales ---
    GtkWidget *controls_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    GtkWidget *start_btn = gtk_button_new_with_label("Comenzar escaneo");
    GtkWidget *stop_btn = gtk_button_new_with_label("Parar escaneo");
    gtk_widget_set_hexpand(start_btn, TRUE);
    gtk_widget_set_hexpand(stop_btn, TRUE);
    gtk_box_pack_start(GTK_BOX(controls_hbox), start_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_hbox), stop_btn, TRUE, TRUE, 0);

    // Empaquetar todo en el vbox principal
    gtk_box_pack_start(GTK_BOX(main_vbox), sections_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), console_frame, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), controls_hbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // Conectar señales
    g_signal_connect(usb_scan_btn, "clicked", G_CALLBACK(on_scan_usb_clicked), NULL);
    g_signal_connect(proc_scan_btn, "clicked", G_CALLBACK(on_scan_processes_clicked), NULL);
    g_signal_connect(ports_scan_btn, "clicked", G_CALLBACK(on_scan_ports_clicked), NULL);
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_scan_clicked), NULL);
    g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_scan_clicked), NULL);
    g_signal_connect(console_entry, "activate", G_CALLBACK(on_console_command_entered), NULL);

    gtk_widget_show_all(window);
    gtk_main();
}