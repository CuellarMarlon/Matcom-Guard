#ifndef THRONE_ROOM_H
#define THRONE_ROOM_H

#include "gui/gui.h"  // Para GuiContext

void controlador_principal(GuiContext *ctx);
void detener_controlador_desde_gui(void);
void controlador_rf1_usb(GuiContext *ctx);
void controlador_rf2_procesos(void);
int  controlador_rf3_puertos(GuiContext *ctx);

#endif