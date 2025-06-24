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
#include <vte/vte.h>

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
        agendar_agregar_texto(global_ctx.console_textview, buffer);
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
        printf(cmd_with_newline, sizeof(cmd_with_newline), "%s\n", command);
        write(pty_master_fd, cmd_with_newline, strlen(cmd_with_newline));
        gtk_entry_set_text(entry, "");
    }
}

static void* rf1_thread_fn(void* arg) {
    controlador_rf1_usb((GuiContext*)arg);
    return NULL;
}

static void* rf2_thread_fn(void* arg) {
    controlador_rf2_procesos();
    return NULL;
}

static void* rf3_thread_fn(void* arg) {
    controlador_rf3_puertos((GuiContext*)arg);
    return NULL;
}

// Limpiar USB
static void on_clear_usb_clicked(GtkButton *button, gpointer user_data) {
    GtkTextView *textview = (GtkTextView*)user_data;
    set_text_to_view(textview, "");
}

// Limpiar Puertos
static void on_clear_ports_clicked(GtkButton *button, gpointer user_data) {
    GtkTextView *textview = (GtkTextView*)user_data;
    set_text_to_view(textview, "");
}

// Limpiar la tabla de procesos
static void on_clear_proc_clicked(GtkButton *button, gpointer user_data) {
    if (process_list_store)
        gtk_list_store_clear(process_list_store);
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

    // Botón limpiar pequeño alineado a la derecha
    GtkWidget *hbox_btn = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *clear_btn = gtk_button_new_with_label("Limpiar");
    gtk_widget_set_size_request(clear_btn, 60, 24);
    gtk_box_pack_end(GTK_BOX(hbox_btn), clear_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), hbox_btn, FALSE, FALSE, 0);

    // Conectar señal según el título
    if (g_strcmp0(title, "Dispositivos USB") == 0)
        g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_usb_clicked), text);
    else
        g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_ports_clicked), text);

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

    // Botón limpiar pequeño alineado a la derecha
    GtkWidget *hbox_btn = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *clear_btn = gtk_button_new_with_label("Limpiar");
    gtk_widget_set_size_request(clear_btn, 60, 24);
    gtk_box_pack_end(GTK_BOX(hbox_btn), clear_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), hbox_btn, FALSE, FALSE, 0);

    // Conectar señal
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_proc_clicked), NULL);

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
        detener_controlador_desde_gui();
        g_print("🛑 Señal de parada enviada al controlador.\n");
        controller_running = 0;
    } else {
        g_print("⚠️ No hay escaneo activo.\n");
    }
}

// Función para cerrar la aplicación
static void on_exit_clicked(GtkButton *button, gpointer user_data) {
    gtk_main_quit();
}

void mostrar_panel_intro(GtkWidget *window_principal) {
    GtkWidget *splash = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(splash), "Bienvenido a MatcomGuard");
    gtk_window_set_default_size(GTK_WINDOW(splash), 400, 200);

    // Poner la ventana splash en pantalla completa
    gtk_window_fullscreen(GTK_WINDOW(splash));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    // Texto grande y centrado usando markup
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label),
        "<span font='32' weight='bold'>¡Bienvenido a MatcomGuard!</span>\n\n"
        "<span font='20'>Protegiendo tu sistema...</span>");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_label_set_xalign(GTK_LABEL(label), 0.5);

    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(splash), vbox);

    gtk_widget_show_all(splash);

    // Después de 3 segundos, cerrar splash y mostrar la ventana principal (con todo su contenido)
    g_timeout_add_seconds(3, (GSourceFunc)gtk_widget_show_all, window_principal);
    g_timeout_add_seconds(3, (GSourceFunc)gtk_widget_destroy, splash);
}

void run_gui() {
    gtk_init(NULL, NULL);

    // --- Ventana principal (no la muestres aún) ---
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gran Salón del Trono");
    gtk_window_fullscreen(GTK_WINDOW(window));
    // gtk_window_maximize(GTK_WINDOW(window));
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

    // --- TERMINAL REAL ---
    GtkWidget *console_frame = gtk_frame_new("Consola Interactiva");
    GtkWidget *console_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    VteTerminal *terminal = VTE_TERMINAL(vte_terminal_new());
    gtk_widget_set_hexpand(GTK_WIDGET(terminal), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(terminal), TRUE);

    // Lanzar bash dentro del terminal
    char *argv[] = {"/bin/bash", NULL};
    vte_terminal_spawn_async(
        terminal,
        VTE_PTY_DEFAULT,
        NULL,       // working directory
        argv,       // command
        NULL,       // env
        0,          // flags
        NULL,       // child setup
        NULL,       // child setup data
        NULL,       // child setup data destroy
        -1,         // timeout
        NULL,       // cancellable
        NULL,       // callback
        NULL        // user_data
    );

    gtk_box_pack_start(GTK_BOX(console_vbox), GTK_WIDGET(terminal), TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(console_frame), console_vbox);

    GtkWidget *controls_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    GtkWidget *start_btn = gtk_button_new_with_label("Comenzar escaneo");
    GtkWidget *stop_btn  = gtk_button_new_with_label("Parar escaneo");
    GtkWidget *exit_btn  = gtk_button_new_with_label("Salir"); // <--- Nuevo botón

    gtk_box_pack_start(GTK_BOX(controls_hbox), start_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_hbox), stop_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(controls_hbox), exit_btn, TRUE, TRUE, 0); // <--- Añadir al hbox

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
    g_signal_connect(exit_btn, "clicked", G_CALLBACK(on_exit_clicked), NULL); // <--- Conectar señal

    // --- INICIA LA TERMINAL EMBEBIDA ---
    start_embedded_shell();

    // Llama primero al panel introductorio:
    mostrar_panel_intro(window);

    gtk_main();
}
