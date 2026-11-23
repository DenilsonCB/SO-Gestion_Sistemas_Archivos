#include "bloque4_montaje_permisos.h"
#include "fs_common.h"

#include <fcntl.h>
#include <sys/statvfs.h>
#include <sys/wait.h>

static int leer_montajes(int imprimir, int *total_out) {
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) {
        perror("fopen /proc/mounts");
        return -1;
    }

    char linea[512];
    int total = 0;
    int mostrados = 0;

    if (imprimir) {
        printf("[Bloque 4] Montaje, Comparticion y Proteccion POSIX\n");
    }

    while (fgets(linea, sizeof(linea), fp) != NULL) {
        char dev[128], mnt[128], fstype[64], opts[128];
        if (sscanf(linea, "%127s %127s %63s %127s", dev, mnt, fstype, opts) == 4) {
            total++;
            int es_clave = (strcmp(mnt, "/") == 0 ||
                            strcmp(mnt, "/proc") == 0 ||
                            strcmp(mnt, "/sys") == 0 ||
                            strcmp(mnt, "/tmp") == 0 ||
                            strcmp(mnt, "/mnt/c") == 0);
            if (imprimir && es_clave && mostrados < 4) {
                char modo[16] = "rw";
                if (strncmp(opts, "ro,", 3) == 0 || strcmp(opts, "ro") == 0) {
                    strcpy(modo, "ro");
                }
                printf("  mount %-10s <- %-14.14s (%s, %s)\n",
                       mnt, dev, fstype, modo);
                mostrados++;
            }
        }
    }
    fclose(fp);

    struct statvfs vfs;
    if (statvfs("/", &vfs) == 0 && imprimir) {
        unsigned long free_mb =
            (unsigned long)((vfs.f_bavail * vfs.f_frsize) / (1024UL * 1024UL));
        printf("  statvfs('/')     : total_mnt=%d | bsize=%lu B | libre=%lu MB\n",
               total, (unsigned long)vfs.f_frsize, free_mb);
    }

    if (total_out) {
        *total_out = total;
    }
    return (total > 0) ? 0 : -1;
}

static int probar_fcntl_fork(const char *dir_base, int imprimir,
                             int *rechazado_out, int *adquirido_out) {
    char ruta[FS_MAX_RUTA];
    snprintf(ruta, sizeof(ruta), "%s/b4_lock.dat", dir_base);

    int fd = open(ruta, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open b4 lock");
        return -1;
    }
    if (write(fd, "registro compartido\n", 20) != 20) {
        close(fd);
        unlink(ruta);
        return -1;
    }

    int p2h[2], h2p[2];
    if (pipe(p2h) != 0 || pipe(h2p) != 0) {
        close(fd);
        unlink(ruta);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(p2h[0]); close(p2h[1]);
        close(h2p[0]); close(h2p[1]);
        close(fd);
        unlink(ruta);
        return -1;
    }

    if (pid == 0) {
        close(p2h[1]);
        close(h2p[0]);
        char sig = 0;
        if (read(p2h[0], &sig, 1) != 1) {
            _exit(1);
        }

        struct flock consulta = {0};
        consulta.l_type = F_WRLCK;
        consulta.l_whence = SEEK_SET;
        fcntl(fd, F_GETLK, &consulta);

        struct flock intento = {0};
        intento.l_type = F_WRLCK;
        intento.l_whence = SEEK_SET;
        errno = 0;
        int rc1 = fcntl(fd, F_SETLK, &intento);
        int rechazado = (rc1 == -1 && (errno == EACCES || errno == EAGAIN) &&
                         consulta.l_type == F_WRLCK);

        int res[2] = { rechazado, 0 };
        if (write(h2p[1], &res[0], sizeof(int)) != (ssize_t)sizeof(int)) {
            _exit(1);
        }

        if (read(p2h[0], &sig, 1) != 1) {
            _exit(1);
        }
        int rc2 = fcntl(fd, F_SETLK, &intento);
        res[1] = (rc2 == 0) ? 1 : 0;
        if (rc2 == 0) {
            intento.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &intento);
        }
        if (write(h2p[1], &res[1], sizeof(int)) != (ssize_t)sizeof(int)) {
            _exit(1);
        }

        close(p2h[0]);
        close(h2p[1]);
        close(fd);
        _exit(0);
    }

    close(p2h[0]);
    close(h2p[1]);

    struct flock fl = {0};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    if (fcntl(fd, F_SETLK, &fl) != 0) {
        perror("fcntl padre lock");
        close(p2h[1]); close(h2p[0]);
        close(fd); unlink(ruta);
        waitpid(pid, NULL, 0);
        return -1;
    }

    char sig = 'L';
    if (write(p2h[1], &sig, 1) != 1) {
        close(p2h[1]); close(h2p[0]);
        close(fd); unlink(ruta);
        waitpid(pid, NULL, 0);
        return -1;
    }

    int rechazado = 0, adquirido = 0;
    if (read(h2p[0], &rechazado, sizeof(int)) != (ssize_t)sizeof(int)) {
        rechazado = 0;
    }

    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);
    sig = 'U';
    if (write(p2h[1], &sig, 1) != 1) {
        adquirido = 0;
    } else if (read(h2p[0], &adquirido, sizeof(int)) != (ssize_t)sizeof(int)) {
        adquirido = 0;
    }

    int status = 0;
    waitpid(pid, &status, 0);
    close(p2h[1]);
    close(h2p[0]);
    close(fd);
    unlink(ruta);

    if (rechazado_out) *rechazado_out = rechazado;
    if (adquirido_out) *adquirido_out = adquirido;

    if (imprimir) {
        printf("  fcntl F_WRLCK    : hijo bloqueado=%s -> tras F_UNLCK=%s\n",
               rechazado ? "EAGAIN/EACCES" : "NO",
               adquirido ? "adquirido" : "FALLO");
    }
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
}

static int probar_permisos(const char *dir_base, int imprimir,
                           int *write_eacces_out) {
    char ruta[FS_MAX_RUTA];
    snprintf(ruta, sizeof(ruta), "%s/b4_perm.txt", dir_base);

    int fd = open(ruta, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    close(fd);

    if (chmod(ruta, 0444) != 0) {
        unlink(ruta);
        return -1;
    }

    struct stat st;
    stat(ruta, &st);
    char modo_str[11];
    fs_formatear_modo(st.st_mode, modo_str);

    int w_ok = access(ruta, W_OK);
    errno = 0;
    int fd_w = open(ruta, O_WRONLY);
    int err = (fd_w < 0) ? errno : 0;
    if (fd_w >= 0) {
        close(fd_w);
    }

    int bloqueado = (geteuid() == 0) ? 1 : (w_ok != 0 && err == EACCES);
    if (write_eacces_out) {
        *write_eacces_out = bloqueado;
    }

    if (imprimir) {
        printf("  chmod 0444       : modo=%s | access(W_OK)=%d | open=%s\n",
               modo_str, w_ok, (err == EACCES) ? "EACCES" : "permitido");
    }

    chmod(ruta, 0644);
    unlink(ruta);
    return 0;
}

static int ejecutar_escenario_b4(const char *dir_base, int imprimir,
                                 int *montajes_leidos, int *lock_rechazado,
                                 int *lock_adquirido, int *write_eacces) {
    if (leer_montajes(imprimir, montajes_leidos) != 0) {
        return -1;
    }
    if (probar_fcntl_fork(dir_base, imprimir, lock_rechazado,
                          lock_adquirido) != 0) {
        return -1;
    }
    if (probar_permisos(dir_base, imprimir, write_eacces) != 0) {
        return -1;
    }
    return 0;
}

int b4_verificar_todo(const char *dir_base, int *montajes_leidos,
                      int *lock_rechazado, int *lock_adquirido,
                      int *write_eacces) {
    return ejecutar_escenario_b4(dir_base, 0, montajes_leidos, lock_rechazado,
                                 lock_adquirido, write_eacces);
}

int ejecutar_bloque4(const char *dir_base) {
    return ejecutar_escenario_b4(dir_base, 1, NULL, NULL, NULL, NULL);
}
