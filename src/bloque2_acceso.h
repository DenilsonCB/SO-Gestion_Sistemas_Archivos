#ifndef BLOQUE2_ACCESO_H
#define BLOQUE2_ACCESO_H

#include <sys/types.h>

#define B2_NUM_REGISTROS 4000
#define B2_NUM_CONSULTAS 100

typedef struct {
    int id;
    char codigo[16];
    double valor;
} Registro;

typedef struct {
    int id;
    off_t offset;
} EntradaIndice;

typedef struct {
    int registros;
    int consultas;
    double us_secuencial;
    double us_directo;
    double us_indexado;
    long lecturas_sec;
    long lecturas_dir;
    long lecturas_idx;
    int suma_control_sec;
    int suma_control_dir;
    int suma_control_idx;
} MetricAcceso;

/* Genera el archivo de registros y mide los tres metodos de acceso. */
int b2_medir_accesos(const char *dir_base, MetricAcceso *out);

/* Ejecuta la demostracion en consola del Bloque 2. */
int ejecutar_bloque2(const char *dir_base);

#endif
