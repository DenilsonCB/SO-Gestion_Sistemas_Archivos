#include "bloque3_directorios.h"
#include "fs_common.h"

#include <fcntl.h>

static int recorrer_arbol(const char *ruta, int nivel, int imprimir,
                          int *conteo) {
    DIR *d = opendir(ruta);
    if (!d) {
        return -1;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        char sub[FS_MAX_RUTA];
        if (snprintf(sub, sizeof(sub), "%s/%s", ruta, ent->d_name) >=
            (int)sizeof(sub)) {
            closedir(d);
            return -1;
        }
        struct stat st;
        if (lstat(sub, &st) != 0) {
            continue;
        }
        if (conteo) {
            (*conteo)++;
        }
        if (imprimir) {
            const char *tag = S_ISDIR(st.st_mode) ? "[DIR]"
                            : (S_ISLNK(st.st_mode) ? "[LNK]" : "[REG]");
            printf("  %*s%s %-16s (ino=%lu, nlink=%lu)\n",
                   nivel * 2, "", tag, ent->d_name,
                   (unsigned long)st.st_ino, (unsigned long)st.st_nlink);
        }
        /* No descender en enlaces simbolicos para evitar ciclos de grafo. */
        if (S_ISDIR(st.st_mode)) {
            recorrer_arbol(sub, nivel + 1, imprimir, conteo);
        }
    }
    closedir(d);
    return 0;
}

static int crear_archivo_texto(const char *ruta, const char *contenido) {
    int fd = open(ruta, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        return -1;
    }
    size_t n = strlen(contenido);
    ssize_t w = write(fd, contenido, n);
    close(fd);
    return (w == (ssize_t)n) ? 0 : -1;
}

static int ejecutar_escenario_b3(const char *dir_base, int imprimir,
                                 ino_t *ino_orig, ino_t *ino_hard,
                                 ino_t *ino_sym, nlink_t *nlink_con_hard,
                                 nlink_t *nlink_tras_unlink,
                                 int *dangling_errno, int *entradas_recorridas) {
    char arbol[256], docs[300], logs[300];
    char orig[384], hard[384], sym[384], sym_dir[384];

    if (snprintf(arbol, sizeof(arbol), "%s/b3_arbol", dir_base) >= (int)sizeof(arbol)) {
        return -1;
    }
    snprintf(docs, sizeof(docs), "%s/docs", arbol);
    snprintf(logs, sizeof(logs), "%s/logs", arbol);
    snprintf(orig, sizeof(orig), "%s/nota.txt", docs);
    snprintf(hard, sizeof(hard), "%s/nota_hard.txt", logs);
    snprintf(sym, sizeof(sym), "%s/nota_sym.txt", arbol);
    snprintf(sym_dir, sizeof(sym_dir), "%s/ref_docs", logs);

    fs_limpiar_recursivo(arbol);
    if (mkdir(arbol, 0755) != 0 || mkdir(docs, 0755) != 0 ||
        mkdir(logs, 0755) != 0) {
        perror("mkdir b3");
        return -1;
    }

    if (crear_archivo_texto(orig, "Estructura de directorios POSIX\n") != 0) {
        fs_limpiar_recursivo(arbol);
        return -1;
    }
    if (link(orig, hard) != 0 || symlink("docs/nota.txt", sym) != 0 ||
        symlink("../docs", sym_dir) != 0) {
        perror("link/symlink b3");
        fs_limpiar_recursivo(arbol);
        return -1;
    }

    struct stat st_o, st_h, st_s;
    if (stat(orig, &st_o) != 0 || stat(hard, &st_h) != 0 ||
        lstat(sym, &st_s) != 0) {
        fs_limpiar_recursivo(arbol);
        return -1;
    }

    int conteo = 0;
    if (imprimir) {
        printf("[Bloque 3] Estructura de Directorios y Enlaces\n");
        recorrer_arbol(arbol, 0, 1, &conteo);
    } else {
        recorrer_arbol(arbol, 0, 0, &conteo);
    }

    char destino[128];
    ssize_t rl = readlink(sym, destino, sizeof(destino) - 1);
    destino[(rl > 0) ? rl : 0] = '\0';

    if (unlink(orig) != 0) {
        fs_limpiar_recursivo(arbol);
        return -1;
    }

    struct stat st_h2, st_roto;
    if (stat(hard, &st_h2) != 0) {
        fs_limpiar_recursivo(arbol);
        return -1;
    }
    errno = 0;
    int rc_roto = stat(sym, &st_roto);
    int err_roto = (rc_roto != 0) ? errno : 0;

    if (ino_orig) *ino_orig = st_o.st_ino;
    if (ino_hard) *ino_hard = st_h.st_ino;
    if (ino_sym) *ino_sym = st_s.st_ino;
    if (nlink_con_hard) *nlink_con_hard = st_h.st_nlink;
    if (nlink_tras_unlink) *nlink_tras_unlink = st_h2.st_nlink;
    if (dangling_errno) *dangling_errno = err_roto;
    if (entradas_recorridas) *entradas_recorridas = conteo;

    if (imprimir) {
        printf("  hard link        : orig ino=%lu == hard ino=%lu (nlink=%lu)\n",
               (unsigned long)st_o.st_ino, (unsigned long)st_h.st_ino,
               (unsigned long)st_h.st_nlink);
        printf("  symlink          : lstat ino=%lu -> '%s'\n",
               (unsigned long)st_s.st_ino, destino);
        printf("  tras unlink orig : hard nlink=%lu | stat(sym)=%s\n",
               (unsigned long)st_h2.st_nlink,
               (err_roto == ENOENT) ? "ENOENT" : strerror(err_roto));
    }

    fs_limpiar_recursivo(arbol);
    return 0;
}

int b3_verificar_enlaces(const char *dir_base, ino_t *ino_orig,
                         ino_t *ino_hard, ino_t *ino_sym,
                         nlink_t *nlink_con_hard, nlink_t *nlink_tras_unlink,
                         int *dangling_errno, int *entradas_recorridas) {
    return ejecutar_escenario_b3(dir_base, 0, ino_orig, ino_hard, ino_sym,
                                 nlink_con_hard, nlink_tras_unlink,
                                 dangling_errno, entradas_recorridas);
}

int ejecutar_bloque3(const char *dir_base) {
    return ejecutar_escenario_b3(dir_base, 1, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL);
}
