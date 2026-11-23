#include "bloque2_acceso.h"
#include "fs_common.h"

#include <fcntl.h>
#include <time.h>

static double diff_us(struct timespec a, struct timespec b) {
    double s = (double)(b.tv_sec - a.tv_sec) * 1e6;
    double ns = (double)(b.tv_nsec - a.tv_nsec) / 1e3;
    return s + ns;
}

static int buscar_indice_binario(const EntradaIndice *idx, int n, int id,
                                 off_t *off_out, long *comparaciones) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (comparaciones) {
            (*comparaciones)++;
        }
        if (idx[mid].id == id) {
            *off_out = idx[mid].offset;
            return 0;
        }
        if (idx[mid].id < id) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

int b2_medir_accesos(const char *dir_base, MetricAcceso *out) {
    char ruta[FS_MAX_RUTA];
    if (snprintf(ruta, sizeof(ruta), "%s/b2_datos.bin", dir_base) >=
        (int)sizeof(ruta)) {
        return -1;
    }

    int fd = open(ruta, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open b2");
        return -1;
    }

    EntradaIndice *indice = calloc(B2_NUM_REGISTROS, sizeof(EntradaIndice));
    if (!indice) {
        close(fd);
        unlink(ruta);
        return -1;
    }

    for (int i = 0; i < B2_NUM_REGISTROS; i++) {
        Registro r;
        memset(&r, 0, sizeof(r));
        r.id = 1000 + i;
        snprintf(r.codigo, sizeof(r.codigo), "REG-%04d", r.id);
        r.valor = (double)r.id * 1.25;
        indice[i].id = r.id;
        indice[i].offset = (off_t)i * (off_t)sizeof(Registro);
        if (write(fd, &r, sizeof(r)) != (ssize_t)sizeof(r)) {
            free(indice);
            close(fd);
            unlink(ruta);
            return -1;
        }
    }

    int claves[B2_NUM_CONSULTAS];
    for (int i = 0; i < B2_NUM_CONSULTAS; i++) {
        claves[i] = 1000 + ((i * 397) % B2_NUM_REGISTROS);
    }

    memset(out, 0, sizeof(*out));
    out->registros = B2_NUM_REGISTROS;
    out->consultas = B2_NUM_CONSULTAS;

    struct timespec t0, t1;
    Registro tmp;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int q = 0; q < B2_NUM_CONSULTAS; q++) {
        lseek(fd, 0, SEEK_SET);
        while (read(fd, &tmp, sizeof(tmp)) == (ssize_t)sizeof(tmp)) {
            out->lecturas_sec++;
            if (tmp.id == claves[q]) {
                out->suma_control_sec += tmp.id;
                break;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    out->us_secuencial = diff_us(t0, t1);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int q = 0; q < B2_NUM_CONSULTAS; q++) {
        off_t pos = (off_t)(claves[q] - 1000) * (off_t)sizeof(Registro);
        lseek(fd, pos, SEEK_SET);
        if (read(fd, &tmp, sizeof(tmp)) == (ssize_t)sizeof(tmp)) {
            out->lecturas_dir++;
            out->suma_control_dir += tmp.id;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    out->us_directo = diff_us(t0, t1);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int q = 0; q < B2_NUM_CONSULTAS; q++) {
        off_t pos = 0;
        if (buscar_indice_binario(indice, B2_NUM_REGISTROS, claves[q], &pos,
                                  NULL) == 0) {
            lseek(fd, pos, SEEK_SET);
            if (read(fd, &tmp, sizeof(tmp)) == (ssize_t)sizeof(tmp)) {
                out->lecturas_idx++;
                out->suma_control_idx += tmp.id;
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    out->us_indexado = diff_us(t0, t1);

    free(indice);
    close(fd);
    unlink(ruta);
    return 0;
}

int ejecutar_bloque2(const char *dir_base) {
    MetricAcceso m;
    if (b2_medir_accesos(dir_base, &m) != 0) {
        return -1;
    }
    printf("[Bloque 2] Metodos de Acceso (%d reg, %d busquedas)\n",
           m.registros, m.consultas);
    printf("  Secuencial O(n)    : %8.1f us | %ld lecturas\n",
           m.us_secuencial, m.lecturas_sec);
    printf("  Directo O(1)       : %8.1f us | %ld lecturas\n",
           m.us_directo, m.lecturas_dir);
    printf("  Indexado O(log n)  : %8.1f us | %ld lecturas\n",
           m.us_indexado, m.lecturas_idx);
    return 0;
}
