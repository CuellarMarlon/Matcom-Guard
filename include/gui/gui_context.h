#ifndef GUI_CONTEXT_H
#define GUI_CONTEXT_H

#include <gtk/gtk.h>

typedef struct {
    GtkTextView* usb_textview;
    GtkTextView* proc_textview;
    GtkTextView* ports_textview;
} GuiContext;

#endif
