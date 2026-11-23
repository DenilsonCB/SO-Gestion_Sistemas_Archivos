#include "src/bloque1_atributos.h"
#include "src/bloque2_acceso.h"
#include "src/bloque3_directorios.h"
#include "src/bloque4_montaje_permisos.h"
#include "src/bloque5_vfs_sim.h"
#include "src/fs_common.h"

static void imprimir_ayuda(const char *prog) {
    printf("Uso: %s [--bloque <1..5> | --ayuda]\n", prog);
    printf("  Sin flags : ejecuta secuencialmente los bloques 1 al 5.\n");
    printf("  --bloque N: ejecuta unicamente el bloque N.\n");
}

static int correr_bloque(int n, const char *sandbox) {
    switch (n) {
    case 1: return ejecutar_bloque1(sandbox);
    case 2: return ejecutar_bloque2(sandbox);
    case 3: return ejecutar_bloque3(sandbox);
    case 4: return ejecutar_bloque4(sandbox);
    case 5: return ejecutar_bloque5();
    default: return -1;
    }
}

int main(int argc, char **argv) {
    int solo_bloque = 0;

    if (argc == 2 && strcmp(argv[1], "--ayuda") == 0) {
        imprimir_ayuda(argv[0]);
        return 0;
    } else if (argc == 3 && strcmp(argv[1], "--bloque") == 0) {
        solo_bloque = atoi(argv[2]);
        if (solo_bloque < 1 || solo_bloque > 5) {
            fprintf(stderr, "Error: bloque invalido (use 1..5).\n");
            return 1;
        }
    } else if (argc != 1) {
        imprimir_ayuda(argv[0]);
        return 1;
    }

    char sandbox[FS_MAX_RUTA];
    if (fs_crear_sandbox(sandbox, sizeof(sandbox)) != 0) {
        return 1;
    }

    int rc = 0;
    if (solo_bloque > 0) {
        rc = correr_bloque(solo_bloque, sandbox);
    } else {
        for (int b = 1; b <= 5; b++) {
            if (correr_bloque(b, sandbox) != 0) {
                rc = 1;
                break;
            }
            if (b < 5) {
                printf("\n");
            }
        }
    }

    fs_limpiar_recursivo(sandbox);
    return (rc == 0) ? 0 : 1;
}
