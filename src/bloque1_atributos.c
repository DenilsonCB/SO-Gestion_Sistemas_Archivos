#include "bloque1_atributos.h"
#include "fs_common.h"

#include <fcntl.h>
#include <time.h>

static const char *b1_tipo_archivo(mode_t m) {
    if (S_ISREG(m)) return "regular";
    if (S_ISDIR(m)) return "directorio";
    if (S_ISLNK(m)) return "enlace simbolico";
    return "otro";
}

int b1_inspeccionar_ciclo_archivo(const char *dir_base,
                                  struct stat *st_antes,
                                  struct stat *st_despues,
                                  int *fd_usado) {
    char ruta[FS_MAX_RUTA];
    const char *msg = "Sistemas Operativos UNSA - Prueba de bloque\n";
    size_t len = strlen(msg);
    char buf[64];

    if (snprintf(ruta, sizeof(ruta), "%s/b1_registro.txt", dir_base) >=
        (int)sizeof(ruta)) {
        return -1;
    }

    int fd = open(ruta, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open b1");
        return -1;
    }
    if (fd_usado) {
        *fd_usado = fd;
    }

    if (write(fd, msg, len) != (ssize_t)len) {
        perror("write b1");
        close(fd);
        unlink(ruta);
        return -1;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("lseek b1");
        close(fd);
        unlink(ruta);
        return -1;
    }

    ssize_t leidos = read(fd, buf, sizeof(buf) - 1);
    if (leidos < 0) {
        perror("read b1");
        close(fd);
        unlink(ruta);
        return -1;
    }
    buf[leidos] = '\0';

    if (lstat(ruta, st_antes) != 0) {
        perror("lstat b1 antes");
        close(fd);
        unlink(ruta);
        return -1;
    }

    if (ftruncate(fd, 20) != 0) {
        perror("ftruncate b1");
        close(fd);
        unlink(ruta);
        return -1;
    }

    if (stat(ruta, st_despues) != 0) {
        perror("stat b1 despues");
        close(fd);
        unlink(ruta);
        return -1;
    }

    close(fd);
    if (unlink(ruta) != 0) {
        perror("unlink b1");
        return -1;
    }
    return 0;
}

int ejecutar_bloque1(const char *dir_base) {
    struct stat antes, despues;
    int fd = -1;
    char permisos[11];
    char fecha[32];

    if (b1_inspeccionar_ciclo_archivo(dir_base, &antes, &despues, &fd) != 0) {
        return -1;
    }

    fs_formatear_modo(antes.st_mode, permisos);
    struct tm *tm_info = localtime(&antes.st_mtime);
    strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("[Bloque 1] Fundamentos, Atributos y Operaciones POSIX\n");
    printf("  fd asignado      : %d\n", fd);
    printf("  i-nodo (st_ino)  : %lu\n", (unsigned long)antes.st_ino);
    printf("  tipo / modo      : %s (%s | 0%03o)\n",
           b1_tipo_archivo(antes.st_mode), permisos,
           (unsigned)(antes.st_mode & 0777));
    printf("  enlaces (nlink)  : %lu\n", (unsigned long)antes.st_nlink);
    printf("  tamano inicial   : %lld bytes (%lld bloques 512B)\n",
           (long long)antes.st_size, (long long)antes.st_blocks);
    printf("  mtime            : %s\n", fecha);
    printf("  tras ftruncate   : %lld bytes (unlink completado)\n",
           (long long)despues.st_size);
    return 0;
}
