#include "src/bloque1_atributos.h"
#include "src/bloque2_acceso.h"
#include "src/bloque3_directorios.h"
#include "src/bloque4_montaje_permisos.h"
#include "src/bloque5_vfs_sim.h"
#include "src/fs_common.h"

static int total_pruebas = 0;
static int pruebas_ok = 0;

static void verificar(int cond, const char *nombre) {
    total_pruebas++;
    if (cond) {
        pruebas_ok++;
        printf("[OK]   %s\n", nombre);
    } else {
        printf("[FAIL] %s\n", nombre);
    }
}

int main(void) {
    char sandbox[FS_MAX_RUTA];
    if (fs_crear_sandbox(sandbox, sizeof(sandbox)) != 0) {
        return 1;
    }

    struct stat b1_a, b1_d;
    int fd1 = -1;
    int rc1 = b1_inspeccionar_ciclo_archivo(sandbox, &b1_a, &b1_d, &fd1);
    verificar(rc1 == 0 && fd1 >= 3 && S_ISREG(b1_a.st_mode),
              "B1: apertura POSIX, descriptor valido y tipo regular");
    verificar(b1_a.st_size > 20 && b1_d.st_size == 20,
              "B1: crecimiento de archivo y truncado exacto a 20 B");

    MetricAcceso m2;
    int rc2 = b2_medir_accesos(sandbox, &m2);
    verificar(rc2 == 0 && m2.suma_control_sec == m2.suma_control_dir &&
                  m2.suma_control_sec == m2.suma_control_idx,
              "B2: equivalencia de suma de control en los 3 accesos");
    verificar(m2.lecturas_dir == B2_NUM_CONSULTAS &&
                  m2.lecturas_idx == B2_NUM_CONSULTAS &&
                  m2.lecturas_sec > m2.lecturas_dir,
              "B2: lectura O(1)/indexada requiere menos I/O que secuencial");

    ino_t orig = 0, hard = 0, sym = 0;
    nlink_t n1 = 0, n2 = 0;
    int dang_err = 0, ents = 0;
    int rc3 = b3_verificar_enlaces(sandbox, &orig, &hard, &sym, &n1, &n2,
                                   &dang_err, &ents);
    verificar(rc3 == 0 && orig == hard && orig != sym,
              "B3: hard link comparte i-nodo y symlink usa i-nodo propio");
    verificar(n1 == 2 && n2 == 1 && dang_err == ENOENT && ents >= 5,
              "B3: nlink 2->1 tras unlink y deteccion ENOENT de dangling link");

    int mnt_n = 0, lock_rej = 0, lock_ok = 0, w_eacces = 0;
    int rc4 = b4_verificar_todo(sandbox, &mnt_n, &lock_rej, &lock_ok, &w_eacces);
    verificar(rc4 == 0 && mnt_n > 0,
              "B4: parseo de puntos de montaje reales desde /proc/mounts");
    verificar(lock_rej == 1 && lock_ok == 1,
              "B4: conflicto fcntl F_WRLCK entre procesos y cesion tras F_UNLCK");
    verificar(w_eacces == 1,
              "B4: rechazo de escritura EACCES tras chmod 0444");

    SuperbloqueVFS sb;
    vfs_init(&sb, "vfs-test", "/");
    int ino = vfs_crear_archivo(&sb, "/doc/info.txt", 0644, "ABCDEFGHIJ");
    int vfd1 = vfs_open(&sb, "/doc/info.txt");
    int vfd2 = vfs_dup(&sb, vfd1);
    int vfd3 = vfs_open(&sb, "/doc/info.txt");
    char b_a[8] = {0}, b_b[8] = {0}, b_c[8] = {0};
    vfs_read(&sb, vfd1, b_a, 3);
    vfs_read(&sb, vfd2, b_b, 3);
    vfs_read(&sb, vfd3, b_c, 3);
    int g1 = sb.tabla_proc[vfd1].idx_global;
    int g3 = sb.tabla_proc[vfd3].idx_global;
    verificar(ino >= 100 && sb.tabla_global[g1].refcount == 2 &&
                  strcmp(b_a, "ABC") == 0 && strcmp(b_b, "DEF") == 0,
              "B5: dup comparte tabla global (refcount=2 y offset comun)");
    verificar(g1 != g3 && sb.tabla_global[g3].refcount == 1 &&
                  strcmp(b_c, "ABC") == 0,
              "B5: segundo open crea offset independiente en tabla global");
    vfs_close(&sb, vfd1);
    vfs_close(&sb, vfd2);
    vfs_close(&sb, vfd3);
    verificar(!sb.tabla_global[g1].usado && !sb.tabla_global[g3].usado,
              "B5: cierre completo libera entradas de tabla global");

    fs_limpiar_recursivo(sandbox);
    printf("\nResumen: %d/%d pruebas superadas.\n", pruebas_ok, total_pruebas);
    return (pruebas_ok == total_pruebas) ? 0 : 1;
}
