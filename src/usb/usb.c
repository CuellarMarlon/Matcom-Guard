// Habilita algunas extensiones POSIX y XOPEN para ciertas funciones como nftw(), strdup(), etc.
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

#include "usb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>           // Para recorrer directorios recursivamente (nftw)
#include <openssl/sha.h>   // Para calcular hash SHA256
#include <sys/stat.h>      // Para estructura stat (permisos, UID, tamaño, etc.)
#include <unistd.h>        // Para funciones POSIX como access()
#include <errno.h>         // Para manejo de errores

// =========================
// Estructura para representar un archivo escaneado
// =========================
typedef struct file_hash {
    char *path;                              // Ruta del archivo
    unsigned char hash[SHA256_DIGEST_LENGTH]; // Hash SHA256 del contenido
    mode_t mode;                             // Permisos (como 0644)
    off_t size;                              // Tamaño del archivo
    uid_t uid;                               // UID del propietario
    gid_t gid;                               // GID del grupo
    time_t mtime;                            // Última modificación
    struct file_hash *next;                  // Puntero al siguiente nodo (lista enlazada)
} file_hash_t;

// =========================
// Estructura principal del "monitor"
// =========================
struct usb_monitor {
    usb_config_t cfg;        // Configuración cargada desde archivo (punto de montaje, umbral, etc.)
    file_hash_t *baseline;   // Lista de archivos conocidos (baseline)
    int baseline_ready;      // Indica si la baseline ya fue creada
    usb_alert_cb callback;   // Función de callback para notificar cambios
    void *cb_data;           // Puntero de datos extra que se pasa al callback
};

// =========================
// Declaraciones de funciones auxiliares
// =========================
static int compute_sha256(const char *path, unsigned char out_hash[SHA256_DIGEST_LENGTH]);
static int process_file(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static file_hash_t* find_hash(file_hash_t *list, const char *path);
static void free_hash_list(file_hash_t *list);

// Lista global temporal para almacenar archivos en cada escaneo
static file_hash_t *current_scan = NULL;

// =========================
// Crea un nuevo monitor con la configuración indicada
// =========================
usb_monitor_t* usb_monitor_create(const usb_config_t *cfg) {
    usb_monitor_t *mon = malloc(sizeof(*mon));
    mon->cfg = *cfg;
    mon->baseline = NULL;
    mon->baseline_ready = 0;
    mon->callback = NULL;
    mon->cb_data = NULL;
    return mon;
}

// =========================
// Registra el callback que se llamará cuando haya cambios
// =========================
void usb_monitor_set_callback(usb_monitor_t *mon, usb_alert_cb on_alert, void *user_data) {
    printf("🧪 usb_monitor_set_callback configurado\n");
    mon->callback = on_alert;
    mon->cb_data = user_data;
}

// =========================
// Libera toda la memoria usada por el monitor
// =========================
void usb_monitor_destroy(usb_monitor_t *mon) {
    if (!mon) return;
    free_hash_list(mon->baseline);
    free(mon);
}

// =========================
// Calcula el hash SHA256 de un archivo
// =========================
static int compute_sha256(const char *path, unsigned char out_hash[SHA256_DIGEST_LENGTH]) {
    FILE *file = fopen(path, "rb");
    if (!file) return -1;

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    unsigned char buffer[8192];
    size_t len;
    while ((len = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        SHA256_Update(&ctx, buffer, len);
    }

    SHA256_Final(out_hash, &ctx);
    fclose(file);
    return 0;
}

// =========================
// Procesa un archivo encontrado por nftw(), guarda sus metadatos en la lista
// =========================
static int process_file(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)ftwbuf;
    if (typeflag != FTW_F) return 0; // Solo archivos regulares

    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (compute_sha256(fpath, hash) != 0) {
        fprintf(stderr, "Error leyendo: %s\n", fpath);
        return 0;
    }

    // Crear nodo con metadatos
    file_hash_t *node = malloc(sizeof(file_hash_t));
    node->path = strdup(fpath);
    memcpy(node->hash, hash, SHA256_DIGEST_LENGTH);
    node->mode = sb->st_mode;
    node->size = sb->st_size;
    node->uid = sb->st_uid;
    node->gid = sb->st_gid;
    node->mtime = sb->st_mtime;
    node->next = current_scan;
    current_scan = node;

    return 0;
}

// =========================
// Busca un archivo por ruta en una lista de file_hash
// =========================
static file_hash_t* find_hash(file_hash_t *list, const char *path) {
    for (; list; list = list->next) {
        if (strcmp(list->path, path) == 0) return list;
    }
    return NULL;
}

// =========================
// Libera toda una lista de file_hash
// =========================
static void free_hash_list(file_hash_t *list) {
    while (list) {
        file_hash_t *next = list->next;
        free(list->path);
        free(list);
        list = next;
    }
}

// =========================
// Variables globales para controlar cambios
// =========================
size_t changed = 0;
size_t changedaux = 0;

// =========================
// Función principal de escaneo de USB: detecta cambios con respecto al baseline
// =========================
void usb_monitor_scan(usb_monitor_t *mon) {
    if (!mon) return;

    char result[8192];
    result[0] = '\0';

    // Verifica si el punto de montaje existe
    if (access(mon->cfg.mount_point, F_OK) != 0) {
        snprintf(result, sizeof(result),
                 "🔌 USB no disponible (%s): %s\n",
                 mon->cfg.mount_point,
                 strerror(errno));
        if (mon->callback) {
            mon->callback(result, mon->cb_data);
        }
        return;
    }

    // Escaneo del filesystem
    current_scan = NULL;
    nftw(mon->cfg.mount_point, process_file, 16, FTW_PHYS);
    file_hash_t *current = current_scan;

    // Si es el primer escaneo, se genera la baseline
    if (!mon->baseline_ready) {
        mon->baseline = current;
        mon->baseline_ready = 1;
        snprintf(result, sizeof(result), "✅ Baseline generado.\n");
        changed = 0;
        if (mon->callback) {
            mon->callback(result, mon->cb_data);
        }
        return;
    }

    size_t total = 0;
    file_hash_t *f;

    // Comparar archivos actuales con la baseline
    for (f = current; f; f = f->next) {
        total++;
        file_hash_t *old = find_hash(mon->baseline, f->path);
        if (!old) {
            changed++;
            snprintf(result + strlen(result), sizeof(result) - strlen(result),
                     "🆕 Nuevo archivo: %s\n", f->path);
        } else {
            // Comparar contenido (hash) y metadatos
            if (memcmp(old->hash, f->hash, SHA256_DIGEST_LENGTH) != 0) {
                if (old->size != f->size) {
                    snprintf(result + strlen(result), sizeof(result) - strlen(result),
                             "📏 Tamaño del archivo modificado: %s\n", f->path);
                } else {
                    snprintf(result + strlen(result), sizeof(result) - strlen(result),
                             "✏️  Contenido del archivo modificado: %s\n", f->path);
                }
                changed++;
            }
            if (old->mode != f->mode) {
                snprintf(result + strlen(result), sizeof(result) - strlen(result),
                         "🔐 Permisos cambiados: %s\n", f->path);
                changed++;
            }
            if (old->uid != f->uid || old->gid != f->gid) {
                snprintf(result + strlen(result), sizeof(result) - strlen(result),
                         "👤 Propietario cambiado: %s (UID: %d → %d, GID: %d → %d)\n",
                         f->path, old->uid, f->uid, old->gid, f->gid);
                changed++;
            }
            if (old->mtime != f->mtime) {
                snprintf(result + strlen(result), sizeof(result) - strlen(result),
                         "🕒 Timestamp modificado: %s\n", f->path);
                changed++;
            }
        }
    }

    // Detectar archivos eliminados
    for (f = mon->baseline; f; f = f->next) {
        if (!find_hash(current, f->path)) {
            snprintf(result + strlen(result), sizeof(result) - strlen(result),
                     "🗑️  Archivo eliminado: %s\n", f->path);
            changed++;
            total++;
        }
    }

    // Reportar resumen
    double ratio = (total > 0) ? ((double)changed / total) : 0;
    if (ratio > mon->cfg.change_threshold) {
        snprintf(result + strlen(result), sizeof(result) - strlen(result),
                 "⚠️  ALERTA: %.2f%% de archivos cambiaron (umbral %.2f%%)\n",
                 ratio * 100, mon->cfg.change_threshold * 100);
        changed = 0;  // reset para evitar acumulación
    } else if (changed > 0 && changed != changedaux) {
        snprintf(result + strlen(result), sizeof(result) - strlen(result),
                 "📊 Cambios detectados: %zu/%zu archivos (%.2f%%)\n",
                 changed, total, ratio * 100);
        changedaux = changed;
    }

    // Enviar resultado a GUI u otro componente
    if (mon->callback) {
        mon->callback(result, mon->cb_data);
    }

    // Actualizar baseline
    free_hash_list(mon->baseline);
    mon->baseline = current;
}
