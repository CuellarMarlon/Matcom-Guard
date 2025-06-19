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
    gtk_window_maximize(GTK_WINDOW(window));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // VBox principal
    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 10);

    // === Sección superior con USB, Procesos y Puertos ===
    GtkWidget *sections_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_hexpand(sections_hbox, TRUE);
    gtk_widget_set_vexpand(sections_hbox, FALSE); // Solo ocupa parte superior

    GtkWidget* create_section(const gchar *title, GtkWidget **text_view_out, GtkWidget **button_out, const gchar *button_label) {
        GtkWidget *frame = gtk_frame_new(title);
        gtk_widget_set_hexpand(frame, TRUE);
        gtk_widget_set_vexpand(frame, TRUE);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        GtkWidget *text = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
        gtk_widget_set_hexpand(text, TRUE);
        gtk_widget_set_vexpand(text, TRUE);

        GtkWidget *button = gtk_button_new_with_label(button_label);
        gtk_box_pack_start(GTK_BOX(vbox), text, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        *text_view_out = text;
        *button_out = button;
        return frame;
    }

    GtkWidget *usb_text, *usb_scan_btn;
    GtkWidget *proc_text, *proc_scan_btn;
    GtkWidget *ports_text, *ports_scan_btn;

    GtkWidget *usb_frame = create_section("Dispositivos USB", &usb_text, &usb_scan_btn, "Escanear USB");
    GtkWidget *proc_frame = create_section("Procesos Monitoreados", &proc_text, &proc_scan_btn, "Escanear Procesos");
    GtkWidget *ports_frame = create_section("Puertos Abiertos", &ports_text, &ports_scan_btn, "Escanear Puertos");

    gtk_box_pack_start(GTK_BOX(sections_hbox), usb_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), proc_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), ports_frame, TRUE, TRUE, 0);

    // === Consola expandida (fila inferior) ===
    GtkWidget *console_frame = gtk_frame_new("Consola Interactiva");
    gtk_widget_set_hexpand(console_frame, TRUE);
    gtk_widget_set_vexpand(console_frame, TRUE); // <- Consola más alta

    GtkWidget *console_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *console_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_text), FALSE);
    gtk_widget_set_hexpand(console_text, TRUE);
    gtk_widget_set_vexpand(console_text, TRUE); // <- TextView crece verticalmente

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
    // Crear GtkPaned vertical para dividir las áreas
    GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    
    // Contenedor para la parte superior (módulos USB, procesos, puertos)
    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(top_box), sections_hbox, TRUE, TRUE, 0);
    
    // Contenedor inferior ya es console_frame
    
    // Añadir ambos al paned
    gtk_paned_pack1(GTK_PANED(vpaned), top_box, TRUE, FALSE);      // 1era parte
    gtk_paned_pack2(GTK_PANED(vpaned), console_frame, TRUE, FALSE); // 2da parte
    
    // Establecer posición inicial del divisor (ej. 40% arriba)
    gtk_paned_set_position(GTK_PANED(vpaned), 300);
    
    // Añadir paned al vbox principal
    gtk_box_pack_start(GTK_BOX(main_vbox), vpaned, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(main_vbox), controls_hbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    // Conexión de señales
    g_signal_connect(usb_scan_btn, "clicked", G_CALLBACK(on_scan_usb_clicked), NULL);
    g_signal_connect(proc_scan_btn, "clicked", G_CALLBACK(on_scan_processes_clicked), NULL);
    g_signal_connect(ports_scan_btn, "clicked", G_CALLBACK(on_scan_ports_clicked), NULL);
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_scan_clicked), NULL);
    g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_scan_clicked), NULL);
    g_signal_connect(console_entry, "activate", G_CALLBACK(on_console_command_entered), NULL);

    gtk_widget_show_all(window);
    gtk_main();
}