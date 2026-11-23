#ifndef BLOQUE1_ATRIBUTOS_H
#define BLOQUE1_ATRIBUTOS_H

#include <sys/stat.h>

/*
 * Ejecuta el ciclo de vida POSIX sobre un archivo (open, write,
 * lseek, read, ftruncate, stat, close, unlink) y devuelve sus
 * metadatos antes y despues del truncado.
 */
int b1_inspeccionar_ciclo_archivo(const char *dir_base,
                                  struct stat *st_antes,
                                  struct stat *st_despues,
                                  int *fd_usado);

/* Ejecuta la demostracion en consola del Bloque 1. */
int ejecutar_bloque1(const char *dir_base);

#endif
