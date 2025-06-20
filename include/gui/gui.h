#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

typedef struct {
    GtkTextView *usb_textview;
    GtkTextView *proc_textview;
    GtkTextView *ports_textview;
    GtkTextView *console_textview;
} GuiContext;

// Prototipos
void run_gui();
void append_text_to_view(GtkTextView *view, const char *msg);
void set_text_to_view(GtkTextView *view, const char *msg);
void detener_controller_desde_gui(void);

#endif
