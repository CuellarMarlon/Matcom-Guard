#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include "ports/analysis_ports/process_lookup.h"
#include <linux/limits.h>

int find_process_by_inode(unsigned long inode, int *pid, char *name, size_t name_len) {
    DIR *proc = opendir("/proc");
    if (!proc) return 0;

    struct dirent *entry;
    char fd_path[PATH_MAX], link_target[PATH_MAX], comm_path[PATH_MAX];
    char inode_str[32];
    snprintf(inode_str, sizeof(inode_str), "socket:[%lu]", inode);

    while ((entry = readdir(proc)) != NULL) {
        char *endptr;
        long curr_pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0' || curr_pid <= 0)
            continue;

        snprintf(fd_path, sizeof(fd_path), "/proc/%ld/fd", curr_pid);
        DIR *fd_dir = opendir(fd_path);
        if (!fd_dir)
            continue;

        struct dirent *fd_entry;
        while ((fd_entry = readdir(fd_dir)) != NULL) {
            snprintf(link_target, sizeof(link_target), "%s/%s", fd_path, fd_entry->d_name);
            char link_buf[PATH_MAX];
            ssize_t len = readlink(link_target, link_buf, sizeof(link_buf) - 1);
            if (len == -1)
                continue;
            link_buf[len] = '\0';
            if (strcmp(link_buf, inode_str) == 0) {
                // Encontrado el proceso
                if (pid) *pid = (int)curr_pid;
                snprintf(comm_path, sizeof(comm_path), "/proc/%ld/comm", curr_pid);
                FILE *f = fopen(comm_path, "r");
                if (f) {
                    if (fgets(name, name_len, f)) {
                        // Elimina salto de linea
                        name[strcspn(name, "\n")] = 0;
                    } else {
                        name[0] = '\0';
                    }
                    fclose(f);
                } else {
                    name[0] = '\0';
                }
                closedir(fd_dir);
                closedir(proc);
                return 1;
            }
        }
        closedir(fd_dir);
    }
    closedir(proc);
    return 0;
}