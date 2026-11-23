#include "bloque5_vfs_sim.h"

#include <stdio.h>
#include <string.h>

static ssize_t memfs_leer(const InodoVFS *inodo, size_t off, char *buf,
                          size_t n) {
    if (!inodo || off >= inodo->size) {
        return 0;
    }
    size_t disp = inodo->size - off;
    size_t copia = (n < disp) ? n : disp;
    memcpy(buf, inodo->datos + off, copia);
    return (ssize_t)copia;
}

static ssize_t memfs_escribir(InodoVFS *inodo, const char *datos, size_t n) {
    if (!inodo || !datos) {
        return -1;
    }
    size_t copia = (n < VFS_MAX_DATA - 1) ? n : (VFS_MAX_DATA - 1);
    memcpy(inodo->datos, datos, copia);
    inodo->datos[copia] = '\0';
    inodo->size = copia;
    return (ssize_t)copia;
}

static const OpsArchivo MEMFS_OPS = {
    .leer = memfs_leer,
    .escribir = memfs_escribir,
};

void vfs_init(SuperbloqueVFS *sb, const char *tipo_fs, const char *montaje) {
    memset(sb, 0, sizeof(*sb));
    snprintf(sb->tipo_fs, sizeof(sb->tipo_fs), "%s", tipo_fs);
    snprintf(sb->punto_montaje, sizeof(sb->punto_montaje), "%s", montaje);
    sb->tam_bloque = 4096;
}

int vfs_crear_archivo(SuperbloqueVFS *sb, const char *ruta, int modo,
                      const char *contenido) {
    int idx_ino = -1, idx_den = -1;
    for (int i = 0; i < VFS_MAX_INODOS; i++) {
        if (!sb->inodos[i].usado) {
            idx_ino = i;
            break;
        }
    }
    for (int i = 0; i < VFS_MAX_DENTRIES; i++) {
        if (!sb->dentries[i].usado) {
            idx_den = i;
            break;
        }
    }
    if (idx_ino < 0 || idx_den < 0) {
        return -1;
    }

    InodoVFS *ino = &sb->inodos[idx_ino];
    ino->usado = 1;
    ino->ino = 100 + idx_ino;
    ino->modo = modo;
    ino->nlink = 1;
    ino->ops = &MEMFS_OPS;
    if (contenido) {
        ino->ops->escribir(ino, contenido, strlen(contenido));
    }

    DentryVFS *den = &sb->dentries[idx_den];
    den->usado = 1;
    snprintf(den->nombre, sizeof(den->nombre), "%s", ruta);
    den->ino = ino->ino;
    return ino->ino;
}

int vfs_resolver_ruta(const SuperbloqueVFS *sb, const char *ruta) {
    for (int i = 0; i < VFS_MAX_DENTRIES; i++) {
        if (sb->dentries[i].usado &&
            strcmp(sb->dentries[i].nombre, ruta) == 0) {
            return sb->dentries[i].ino;
        }
    }
    return -1;
}

static InodoVFS *buscar_inodo(SuperbloqueVFS *sb, int ino_num) {
    for (int i = 0; i < VFS_MAX_INODOS; i++) {
        if (sb->inodos[i].usado && sb->inodos[i].ino == ino_num) {
            return &sb->inodos[i];
        }
    }
    return NULL;
}

int vfs_open(SuperbloqueVFS *sb, const char *ruta) {
    int ino_num = vfs_resolver_ruta(sb, ruta);
    if (ino_num < 0) {
        return -1;
    }
    int idx_g = -1, fd = -1;
    for (int i = 0; i < VFS_MAX_GLOBAL; i++) {
        if (!sb->tabla_global[i].usado) {
            idx_g = i;
            break;
        }
    }
    for (int i = 0; i < VFS_MAX_FD; i++) {
        if (!sb->tabla_proc[i].abierto) {
            fd = i;
            break;
        }
    }
    if (idx_g < 0 || fd < 0) {
        return -1;
    }

    sb->tabla_global[idx_g].usado = 1;
    sb->tabla_global[idx_g].ino = ino_num;
    sb->tabla_global[idx_g].offset = 0;
    sb->tabla_global[idx_g].refcount = 1;

    sb->tabla_proc[fd].abierto = 1;
    sb->tabla_proc[fd].idx_global = idx_g;
    return fd;
}

int vfs_dup(SuperbloqueVFS *sb, int fd) {
    if (fd < 0 || fd >= VFS_MAX_FD || !sb->tabla_proc[fd].abierto) {
        return -1;
    }
    int nuevo_fd = -1;
    for (int i = 0; i < VFS_MAX_FD; i++) {
        if (!sb->tabla_proc[i].abierto) {
            nuevo_fd = i;
            break;
        }
    }
    if (nuevo_fd < 0) {
        return -1;
    }
    int idx_g = sb->tabla_proc[fd].idx_global;
    sb->tabla_global[idx_g].refcount++;
    sb->tabla_proc[nuevo_fd].abierto = 1;
    sb->tabla_proc[nuevo_fd].idx_global = idx_g;
    return nuevo_fd;
}

ssize_t vfs_read(SuperbloqueVFS *sb, int fd, char *buf, size_t n) {
    if (fd < 0 || fd >= VFS_MAX_FD || !sb->tabla_proc[fd].abierto) {
        return -1;
    }
    EntradaTablaGlobal *eg = &sb->tabla_global[sb->tabla_proc[fd].idx_global];
    InodoVFS *ino = buscar_inodo(sb, eg->ino);
    if (!ino || !ino->ops || !ino->ops->leer) {
        return -1;
    }
    ssize_t leidos = ino->ops->leer(ino, eg->offset, buf, n);
    if (leidos > 0) {
        eg->offset += (size_t)leidos;
    }
    return leidos;
}

int vfs_close(SuperbloqueVFS *sb, int fd) {
    if (fd < 0 || fd >= VFS_MAX_FD || !sb->tabla_proc[fd].abierto) {
        return -1;
    }
    int idx_g = sb->tabla_proc[fd].idx_global;
    sb->tabla_proc[fd].abierto = 0;
    sb->tabla_global[idx_g].refcount--;
    if (sb->tabla_global[idx_g].refcount <= 0) {
        sb->tabla_global[idx_g].usado = 0;
    }
    return 0;
}

int ejecutar_bloque5(void) {
    SuperbloqueVFS sb;
    vfs_init(&sb, "vfs-sim", "/");
    int ino = vfs_crear_archivo(&sb, "/datos/notas.txt", 0644,
                                "VFS->dentry->inodo->tabla_global");
    int fd1 = vfs_open(&sb, "/datos/notas.txt");
    int fd2 = vfs_dup(&sb, fd1);
    int fd3 = vfs_open(&sb, "/datos/notas.txt");

    char p1[16] = {0}, p2[16] = {0}, p3[16] = {0};
    vfs_read(&sb, fd1, p1, 5);
    vfs_read(&sb, fd2, p2, 6);
    vfs_read(&sb, fd3, p3, 5);

    int g1 = sb.tabla_proc[fd1].idx_global;
    int g3 = sb.tabla_proc[fd3].idx_global;

    printf("[Bloque 5] Capas VFS y Estructuras de Control\n");
    printf("  superbloque      : fs=%s | mnt=%s | blk=%zu B\n",
           sb.tipo_fs, sb.punto_montaje, sb.tam_bloque);
    printf("  dentry->inodo    : /datos/notas.txt -> ino=%d\n", ino);
    printf("  fd1+dup(fd2)     : global[%d] ref=%d off=%zu ('%s'|'%s')\n",
           g1, sb.tabla_global[g1].refcount,
           sb.tabla_global[g1].offset, p1, p2);
    printf("  open ind.(fd3)   : global[%d] ref=%d off=%zu ('%s')\n",
           g3, sb.tabla_global[g3].refcount,
           sb.tabla_global[g3].offset, p3);

    vfs_close(&sb, fd1);
    vfs_close(&sb, fd2);
    vfs_close(&sb, fd3);
    return 0;
}
