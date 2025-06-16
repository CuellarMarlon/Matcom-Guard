#ifndef USB_H
#define USB_H

#include <stddef.h>
#include <openssl/sha.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuración del módulo USB
typedef struct {
    char   *mount_point;     // Ruta donde se monta el USB, e.g. "/mnt/usb"
    double  change_threshold;// Umbral de % de archivos cambiados (0.1 = 10%)
    int     scan_interval;   // Intervalo en segundos entre escaneos
} usb_config_t;

// Tipo opaco para el monitor USB
typedef struct usb_monitor usb_monitor_t;

// Prototipo de callback de alerta
// Recibe un mensaje formateado y el puntero user_data que pasaste al registrar el callback
typedef void (*usb_alert_cb)(const char *message, void *user_data);

// API pública para RF1:
// Crea y devuelve un monitor USB configurado según cfg.
// Debes destruirlo con usb_monitor_destroy() para liberar recursos.
usb_monitor_t* usb_monitor_create(const usb_config_t *cfg);

// Destroye el monitor y libera toda memoria asociada.
void usb_monitor_destroy(usb_monitor_t *mon);

// Registra la función que recibirá las alertas (nuevos/modificados/eliminados)
void usb_monitor_set_callback(usb_monitor_t *mon,
                              usb_alert_cb on_alert,
                              void *user_data);

// Efectúa un escaneo completo del punto de montaje:
//  - Si es el primer escaneo, genera el baseline interno.
//  - En escaneos posteriores, detecta cambios y dispara alertas.
void usb_monitor_scan(usb_monitor_t *mon);

// ------------------------------------------------------------------------------------------------
// Función auxiliar para cargar usb_config_t desde un INI (config/matcomguard.conf).
// Devuelve 0 si se cargó correctamente, -1 en error.
int load_usb_config(const char* path, usb_config_t* out_cfg);

#ifdef __cplusplus
}
#endif

#endif // USB_H
