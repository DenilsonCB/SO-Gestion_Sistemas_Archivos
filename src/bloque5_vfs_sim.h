#ifndef BLOQUE5_VFS_SIM_H
#define BLOQUE5_VFS_SIM_H

#include <stddef.h>
#include <sys/types.h>

#define VFS_MAX_INODOS   16
#define VFS_MAX_DENTRIES 16
#define VFS_MAX_GLOBAL   16
#define VFS_MAX_FD       16
#define VFS_MAX_DATA     128

typedef struct InodoVFS InodoVFS;

typedef struct {
    ssize_t (*leer)(const InodoVFS *inodo, size_t off, char *buf, size_t n);
    ssize_t (*escribir)(InodoVFS *inodo, const char *datos, size_t n);
} OpsArchivo;

struct InodoVFS {
    int usado;
    int ino;
    int modo;
    int nlink;
    size_t size;
    char datos[VFS_MAX_DATA];
    const OpsArchivo *ops;
};

typedef struct {
    int usado;
    char nombre[64];
    int ino;
} DentryVFS;

typedef struct {
    int usado;
    int ino;
    size_t offset;
    int refcount;
} EntradaTablaGlobal;

typedef struct {
    int abierto;
    int idx_global;
} EntradaFdProceso;

typedef struct {
    char tipo_fs[16];
    char punto_montaje[32];
    size_t tam_bloque;
    InodoVFS inodos[VFS_MAX_INODOS];
    DentryVFS dentries[VFS_MAX_DENTRIES];
    EntradaTablaGlobal tabla_global[VFS_MAX_GLOBAL];
    EntradaFdProceso tabla_proc[VFS_MAX_FD];
} SuperbloqueVFS;

void vfs_init(SuperbloqueVFS *sb, const char *tipo_fs, const char *montaje);
int vfs_crear_archivo(SuperbloqueVFS *sb, const char *ruta, int modo,
                      const char *contenido);
int vfs_resolver_ruta(const SuperbloqueVFS *sb, const char *ruta);
int vfs_open(SuperbloqueVFS *sb, const char *ruta);
int vfs_dup(SuperbloqueVFS *sb, int fd);
ssize_t vfs_read(SuperbloqueVFS *sb, int fd, char *buf, size_t n);
int vfs_close(SuperbloqueVFS *sb, int fd);
int ejecutar_bloque5(void);

#endif
