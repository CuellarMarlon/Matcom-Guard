#include <gtk/gtk.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pty.h>
#include <errno.h>
#include <glib.h>
#include "gui/gui.h"
#include "great_throne_room/throne_room.h"
#include "processes.h"

GuiContext global_ctx;
static pthread_t rf1_thread, rf2_thread, rf3_thread;
static int controller_running = 0;

GtkListStore *process_list_store = NULL;

// --- Terminal embebida ---
static int pty_master_fd = -1;
static pid_t shell_pid = -1;
static pthread_t pty_reader_thread;

// Aplica fuente monoespaciada y tamaño grande a un GtkTextView
static void style_textview(GtkWidget *textview) {
    PangoFontDescription *font_desc = pango_font_description_from_string("monospace 13");
    gtk_widget_override_font(textview, font_desc);
    pango_font_description_free(font_desc);
}

// Lee la salida del shell y la muestra en la consola de la GUI
static void* pty_output_reader(void* arg) {
    char buffer[512];
    ssize_t n;
    while ((n = read(pty_master_fd, buffer, sizeof(buffer)-1)) > 0) {
        buffer[n] = '\0';
        schedule_append_text(global_ctx.console_textview, buffer);
    }
    return NULL;
}

// Inicializa el shell en un pty y lanza el hilo lector
static void start_embedded_shell() {
    if (pty_master_fd != -1) return; // Ya iniciado

    int master, slave;
    char slave_name[100];
    struct winsize ws = {24, 80, 0, 0};

    if (openpty(&master, &slave, slave_name, NULL, &ws) == -1) {
        perror("openpty");
        return;
    }

    shell_pid = fork();
    if (shell_pid == 0) {
        // Hijo: shell
        close(master);
        setsid();
        ioctl(slave, TIOCSCTTY, 0);
        dup2(slave, 0);
        dup2(slave, 1);
        dup2(slave, 2);
        close(slave);
        chdir("/home/marlon/University/Second Year/Second Semester/Operative Systems/06. Project/Matcom-Guard"); // Cambia a tu ruta
        execl("/bin/bash", "bash", NULL);
        _exit(127);
    } else if (shell_pid > 0) {
        // Padre: GUI
        close(slave);
        pty_master_fd = master;
        pthread_create(&pty_reader_thread, NULL, pty_output_reader, NULL);
    } else {
        perror("fork");
        close(master);
        close(slave);
    }
}

// Envía el comando al shell embebido
static void on_console_command_entered(GtkEntry *entry, gpointer user_data) {
    const gchar *command = gtk_entry_get_text(entry);
    if (command && *command) {
        char cmd_with_newline[1024];
        snprintf(cmd_with_newline, sizeof(cmd_with_newline), "%s\n", command);
        write(pty_master_fd, cmd_with_newline, strlen(cmd_with_newline));
        gtk_entry_set_text(entry, "");
    }
}

static void* rf1_thread_fn(void* arg) {
    controlador_rf1_usb((GuiContext*)arg);
    return NULL;
}

static void* rf2_thread_fn(void* arg) {
    controlador_rf2_processes();
    return NULL;
}

static void* rf3_thread_fn(void* arg) {
    controlador_rf3_ports((GuiContext*)arg);
    return NULL;
}

// Sección scrollable con GtkTextView (para USB y Puertos)
static GtkWidget* create_section_textview(const gchar *title, GtkTextView **out_view) {
    GtkWidget *frame = gtk_frame_new(title);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_widget_set_hexpand(text, TRUE);
    gtk_widget_set_vexpand(text, TRUE);

    style_textview(text);

    gtk_container_add(GTK_CONTAINER(scrolled), text);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    *out_view = GTK_TEXT_VIEW(text);
    return frame;
}

// Sección de procesos con TreeView scrollable
static GtkWidget* create_section_process_table(GtkWidget **out_treeview) {
    GtkWidget *frame = gtk_frame_new("Procesos Monitoreados");
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    process_list_store = gtk_list_store_new(5,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    GtkWidget *treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(process_list_store));
    crear_columnas(treeview);
    gtk_widget_set_hexpand(treeview, TRUE);
    gtk_widget_set_vexpand(treeview, TRUE);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), treeview);

    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    *out_treeview = treeview;
    return frame;
}

static void on_start_scan_clicked(GtkButton *button, gpointer user_data) {
    if (controller_running) {
        g_print("⚠️ Ya hay un escaneo activo.\n");
        return;
    }

    set_text_to_view(global_ctx.usb_textview, "");
    set_text_to_view(global_ctx.console_textview, "");
    gtk_list_store_clear(process_list_store);

    controller_running = 1;

    if (pthread_create(&rf1_thread, NULL, rf1_thread_fn, &global_ctx) != 0)
        perror("❌ Error al crear hilo para RF1");

    if (pthread_create(&rf2_thread, NULL, rf2_thread_fn, &global_ctx) != 0)
        perror("❌ Error al crear hilo para RF2");

    if (pthread_create(&rf3_thread, NULL, rf3_thread_fn, &global_ctx) != 0)
        perror("❌ Error al crear hilo para RF3");
             

    g_timeout_add_seconds(1, actualizar_lista_gui, NULL);
    g_print("🟢 Escaneo iniciado\n");
}

static void on_stop_scan_clicked(GtkButton *button, gpointer user_data) {
    if (controller_running) {
        detener_controller_desde_gui();
        g_print("🛑 Señal de parada enviada al controlador.\n");
        controller_running = 0;
    } else {
        g_print("⚠️ No hay escaneo activo.\n");
    }
}

void run_gui() {
    gtk_init(NULL, NULL);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gran Salón del Trono");
    gtk_window_maximize(GTK_WINDOW(window));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 10);

    GtkWidget *sections_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget *usb_frame   = create_section_textview("Dispositivos USB", &global_ctx.usb_textview);
    GtkWidget *proc_frame  = create_section_process_table(&global_ctx.proc_textview);
    GtkWidget *ports_frame = create_section_textview("Puertos Abiertos", &global_ctx.ports_textview);

    gtk_box_pack_start(GTK_BOX(sections_hbox), usb_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), proc_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(sections_hbox), ports_frame, TRUE, TRUE, 0);

    // Consola con scroll
    GtkWidget *console_frame = gtk_frame_new("Consola Interactiva");
    GtkWidget *console_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *console_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(console_scrolled),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *console_text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(console_text), FALSE);
    style_textview(console_text);
    gtk_container_add(GTK_CONTAINER(console_scrolled), console_text);

    global_ctx.console_textview = GTK_TEXT_VIEW(console_text);

    GtkWidget *console_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(console_entry), "Escribe un comando...");

    gtk_box_pack_start(GTK_BOX(console_vbox), console_scrolled, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(console_vbox), console_entry, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(console_frame), console_vbox);

    GtkWidget *controls_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    GtkWidget *start_btn = gtk_button_new_with_label("Comenzar escaneo");
    GtkWidget *stop_btn  = gtk_button_new_with_label("Parar escaneo");

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

    // --- INICIA LA TERMINAL EMBEBIDA ---
    start_embedded_shell();

    gtk_widget_show_all(window);
    gtk_main();
}