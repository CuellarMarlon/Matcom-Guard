#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "gui/gui.h"  // asegúrate de que tienes acceso a GuiContext

void controller(GuiContext *ctx);

void detener_controller_desde_gui(void);  // <-- Agrega esta línea

#endif
