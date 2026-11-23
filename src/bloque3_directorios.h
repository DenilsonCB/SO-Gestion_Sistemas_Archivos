#ifndef BLOQUE3_DIRECTORIOS_H
#define BLOQUE3_DIRECTORIOS_H

#include <sys/types.h>

/*
 * Crea un arbol de directorios, enlaces duros y simbolicos,
 * verifica i-nodos, contador st_nlink y comportamiento de enlace roto.
 */
int b3_verificar_enlaces(const char *dir_base,
                         ino_t *ino_orig,
                         ino_t *ino_hard,
                         ino_t *ino_sym,
                         nlink_t *nlink_con_hard,
                         nlink_t *nlink_tras_unlink,
                         int *dangling_errno,
                         int *entradas_recorridas);

/* Ejecuta la demostracion en consola del Bloque 3. */
int ejecutar_bloque3(const char *dir_base);

#endif
