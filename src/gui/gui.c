#include <gtk/gtk.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "gui/gui.h"
#include "great_throne_room/throne_room.h"

GuiContext global_ctx;
static pthread_t controller_thread;
static int controller_running = 0;

// Hilo que ejecuta el controller
static void* controller_thread_fn(void* arg) {
    controller((GuiContext*)arg);
    return NULL;
}

static void* rf1_thread_fn(void* arg) {
    printf("🧵 Hilo RF1 iniciado\n");
    controlador_rf1_usb((GuiContext*)arg);
    return NULL;
}

static void on_start_scan_clicked(GtkButton *button, gpointer user_data) {
    if (controller_running) {
        g_print("⚠️ Ya hay un escaneo activo.\n");
        return;
    }

    set_text_to_view(global_ctx.usb_textview, "");
    set_text_to_view(global_ctx.proc_textview, "");
    set_text_to_view(global_ctx.ports_textview, "");

    controller_running = 1;

    // CAMBIAR ESTO:
    // pthread_create(&controller_thread, NULL, controller_thread_fn, &global_ctx);

    // POR ESTO:
    static pthread_t rf1_thread;
    if (pthread_create(&rf1_thread, NULL, rf1_thread_fn, &global_ctx) != 0) {
        perror("❌ Error al crear hilo para RF1");
        controller_running = 0;
    } else {
        g_print("🟢 Hilo RF1 iniciado correctamente\n");
    }
}


static void on_stop_scan_clicked(GtkButton *button, gpointer user_data) {
    if (controller_running) {
        detener_controller_desde_gui();  // mata a los procesos hijos
        g_print("🛑 Señal de parada enviada al controlador.\n");
        controller_running = 0;
    } else {
        g_print("⚠️ No hay escaneo activo.\n");
    }
}

static void on_console_command_entered(GtkEntry *entry, gpointer user_data) {
    const gchar *command = gtk_entry_get_text(entry);
    if (command && *command) {
        append_text_to_view(global_ctx.console_textview, command);
        gtk_entry_set_text(entry, "");
    }
}

// Función principal que arranca la interfaz gráfica
void run_gui() {
    gtk_init(NULL, NULL);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gran Salón del Trono");
    gtk_window_maximize(GTK_WINDOW(window));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 10);

    GtkWidget *sections_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget* create_section(const gchar *title, GtkTextView **text_view_out) {
        GtkWidget *frame = gtk_frame_new(title);
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        GtkWidget *text = gtk_text_view_new();

        gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
        gtk_widget_set_vexpand(text, TRUE);
        gtk_widget_set_hexpand(text, TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), text, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        *text_view_out = GTK_TEXT_VIEW(text);
        return frame;
    }

    GtkWidget *usb_frame = create_section("Dispositivos USB", &global_ctx.usb_textview);
    GtkWidget *proc_frame = create_section("Procesos Monitoreados", &global_ctx.proc_textview);
    GtkWidget *ports_frame = create_section("Puertos Abiertos", &global_ctx.ports_textview);

    gtk_box_pack_start(GTK_BOX(sections_hbox), usb_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), proc_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), ports_frame, TRUE, TRUE, 0);

    GtkWidget *console_frame = gtk_frame_new("Consola Interactiva");
    GtkWidget *console_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *console_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_text), FALSE);
    global_ctx.console_textview = GTK_TEXT_VIEW(console_text);

    GtkWidget *console_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(console_entry), "Escribe un comando...");

    gtk_box_pack_start(GTK_BOX(console_vbox), console_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(console_vbox), console_entry, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(console_frame), console_vbox);

    GtkWidget *controls_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    GtkWidget *start_btn = gtk_button_new_with_label("Comenzar escaneo");
    GtkWidget *stop_btn = gtk_button_new_with_label("Parar escaneo");

    gtk_box_pack_start(GTK_BOX(controls_hbox), start_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_hbox), stop_btn, TRUE, TRUE, 0);

    GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    GtkWidget *top_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(top_box), sections_hbox, TRUE, TRUE, 0);
    gtk_paned_pack1(GTK_PANED(vpaned), top_box, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(vpaned), console_frame, TRUE, FALSE);
    gtk_paned_set_position(GTK_PANED(vpaned), 520);

    gtk_box_pack_start(GTK_BOX(main_vbox), vpaned, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), controls_hbox, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_scan_clicked), NULL);
    g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_scan_clicked), NULL);
    g_signal_connect(console_entry, "activate", G_CALLBACK(on_console_command_entered), NULL);

    gtk_widget_show_all(window);
    gtk_main();
}
