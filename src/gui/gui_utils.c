#include <gtk/gtk.h>
#include "gui/gui.h"

void append_text_to_view(GtkTextView *view, const char *msg) {
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, msg, -1);
   
}
void set_text_to_view(GtkTextView *view, const char *msg) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
    gtk_text_buffer_set_text(buffer, msg, -1);
}
