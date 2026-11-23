#ifndef BLOQUE4_MONTAJE_PERMISOS_H
#define BLOQUE4_MONTAJE_PERMISOS_H

/*
 * Verifica lectura de /proc/mounts, consulta statvfs("/"),
 * bloqueo fcntl entre procesos (fork) y permisos POSIX sin sudo.
 */
int b4_verificar_todo(const char *dir_base,
                      int *montajes_leidos,
                      int *lock_rechazado,
                      int *lock_adquirido,
                      int *write_eacces);

/* Ejecuta la demostracion en consola del Bloque 4. */
int ejecutar_bloque4(const char *dir_base);

#endif
