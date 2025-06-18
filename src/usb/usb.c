#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

#include "usb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Lista enlazada para almacenar hashes
typedef struct file_hash {
    char *path;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    mode_t mode;
    off_t size;
    struct file_hash *next;
} file_hash_t;

struct usb_monitor {
    usb_config_t cfg;
    file_hash_t *baseline;
    int baseline_ready;
    usb_alert_cb callback;
    void *cb_data;
};

static int compute_sha256(const char *path, unsigned char out_hash[SHA256_DIGEST_LENGTH]);
static int process_file(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
static void print_hash(const unsigned char *hash);
static file_hash_t* find_hash(file_hash_t *list, const char *path);
static void free_hash_list(file_hash_t *list);

static file_hash_t *current_scan = NULL;

usb_monitor_t* usb_monitor_create(const usb_config_t *cfg) {
    usb_monitor_t *mon = malloc(sizeof(*mon));
    mon->cfg = *cfg;
    mon->baseline = NULL;
    mon->baseline_ready = 0;
    mon->callback = NULL;
    mon->cb_data = NULL;
    return mon;
}

void usb_monitor_set_callback(usb_monitor_t *mon, usb_alert_cb on_alert, void *user_data) {
    mon->callback = on_alert;
    mon->cb_data = user_data;
}

void usb_monitor_destroy(usb_monitor_t *mon) {
    if (!mon) return;
    free_hash_list(mon->baseline);
    free(mon);
}

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

static void print_hash(const unsigned char *hash) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

static int process_file(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)ftwbuf;
    if (typeflag != FTW_F) return 0;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (compute_sha256(fpath, hash) != 0) {
        fprintf(stderr, "Error leyendo: %s\n", fpath);
        return 0;
    }

    file_hash_t *node = malloc(sizeof(file_hash_t));
    node->path = strdup(fpath);
    memcpy(node->hash, hash, SHA256_DIGEST_LENGTH);
    node->mode = sb->st_mode;
    node->size = sb->st_size;
    node->next = current_scan;
    current_scan = node;

    // printf("Hash de %s: ", fpath);
    // print_hash(hash);
    return 0;
}

static file_hash_t* find_hash(file_hash_t *list, const char *path) {
    for (; list; list = list->next) {
        if (strcmp(list->path, path) == 0) return list;
    }
    return NULL;
}

static void free_hash_list(file_hash_t *list) {
    while (list) {
        file_hash_t *next = list->next;
        free(list->path);
        free(list);
        list = next;
    }
}

void usb_monitor_scan(usb_monitor_t *mon) {
    if (!mon) return;

    if (access(mon->cfg.mount_point, F_OK) != 0) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "🔌 USB no disponible (%s): %s",
                 mon->cfg.mount_point,
                 strerror(errno));
        printf("%s\n", msg);
        if (mon->callback) {
            mon->callback(msg, mon->cb_data);
        }
        return;
    }

    current_scan = NULL;
    nftw(mon->cfg.mount_point, process_file, 16, FTW_PHYS);
    file_hash_t *current = current_scan;

    if (!mon->baseline_ready) {
        mon->baseline = current;
        mon->baseline_ready = 1;
        printf("✅ Baseline generado.\n");
        return;
    }

    size_t changed = 0, total = 0;
    file_hash_t *f;

    for (f = current; f; f = f->next) {
        total++;
        file_hash_t *old = find_hash(mon->baseline, f->path);
        if (!old) {
            printf("🆕 Nuevo archivo: %s\n", f->path);
            changed++;
        } else {
            if (memcmp(old->hash, f->hash, SHA256_DIGEST_LENGTH) != 0) {
                if (old->size != f->size) {
                    printf("📏 Tamaño del archivo modificado: %s\n", f->path);
                } else {
                    printf("✏️  Contenido del archivo modificado: %s\n", f->path);
                }
                changed++;
            } else if (old->mode != f->mode) {
                printf("🔐 Permisos cambiados: %s\n", f->path);
                changed++;
            }
        }
    }

    for (f = mon->baseline; f; f = f->next) {
        if (!find_hash(current, f->path)) {
            printf("🗑️  Archivo eliminado: %s\n", f->path);
            changed++;
            total++;
        }
    }

    double ratio = (total > 0) ? ((double)changed / total) : 0;
    if (ratio > mon->cfg.change_threshold) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "⚠️  ALERTA: %.2f%% de archivos cambiaron (umbral %.2f%%)",
                 ratio * 100, mon->cfg.change_threshold * 100);
        printf("%s\n", msg);
        if (mon->callback) {
            mon->callback(msg, mon->cb_data);
        }
    }

    free_hash_list(mon->baseline);
    mon->baseline = current;
}
