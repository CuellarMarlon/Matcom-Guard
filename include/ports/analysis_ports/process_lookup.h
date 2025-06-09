#ifndef PROCESS_LOOKUP_H
#define PROCESS_LOOKUP_H

#include <stddef.h>

// Devuelve 1 si encuentra el proceso, 0 si no.
// name debe tener espacio suficiente (ej: 256 bytes).
int find_process_by_inode(unsigned long inode, int *pid, char *name, size_t name_len);

#endif // PROCESS_LOOKUP_H