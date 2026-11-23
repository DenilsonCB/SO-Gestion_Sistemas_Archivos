#ifndef FS_COMMON_H
#define FS_COMMON_H

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FS_MAX_RUTA 512

static inline int fs_crear_sandbox(char *buf, size_t tam) {
    int n = snprintf(buf, tam, "/tmp/so_fs_demo_%ld", (long)getpid());
    if (n < 0 || (size_t)n >= tam) {
        return -1;
    }
    if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
        perror("mkdir sandbox");
        return -1;
    }
    return 0;
}

static inline int fs_limpiar_recursivo(const char *ruta) {
    struct stat st;
    if (lstat(ruta, &st) != 0) {
        return (errno == ENOENT) ? 0 : -1;
    }
    if (!S_ISDIR(st.st_mode)) {
        return unlink(ruta);
    }
    chmod(ruta, 0755);
    DIR *dir = opendir(ruta);
    if (!dir) {
        return -1;
    }
    struct dirent *ent;
    int rc = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        char sub[FS_MAX_RUTA];
        int n = snprintf(sub, sizeof(sub), "%s/%s", ruta, ent->d_name);
        if (n < 0 || (size_t)n >= sizeof(sub)) {
            rc = -1;
            break;
        }
        if (fs_limpiar_recursivo(sub) != 0) {
            rc = -1;
        }
    }
    closedir(dir);
    if (rmdir(ruta) != 0) {
        rc = -1;
    }
    return rc;
}

#ifdef S_ISVTX
#define FS_STICKY_BIT S_ISVTX
#else
#define FS_STICKY_BIT ((mode_t)01000)
#endif

static inline void fs_formatear_modo(mode_t m, char out[11]) {
    out[0] = S_ISDIR(m) ? 'd' : (S_ISLNK(m) ? 'l' : '-');
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_ISUID) ? ((m & S_IXUSR) ? 's' : 'S')
                           : ((m & S_IXUSR) ? 'x' : '-');
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_ISGID) ? ((m & S_IXGRP) ? 's' : 'S')
                           : ((m & S_IXGRP) ? 'x' : '-');
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & FS_STICKY_BIT) ? ((m & S_IXOTH) ? 't' : 'T')
                                 : ((m & S_IXOTH) ? 'x' : '-');
    out[10] = '\0';
}

#endif
