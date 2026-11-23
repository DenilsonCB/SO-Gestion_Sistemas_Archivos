# Gestion de sistemas de archivos: fundamentos y estructura

Implementacion modular en C11/POSIX para demostrar los conceptos
fundamentales y la estructura interna de un sistema de archivos bajo Linux
(WSL Ubuntu). No requiere privilegios de administrador (`sudo`).

## Modulos del proyecto

- `src/bloque1_atributos.c`: descriptor de archivo, metadatos con `stat`/`lstat`, `lseek` y `ftruncate`.
- `src/bloque2_acceso.c`: comparativa temporal y de lecturas entre acceso secuencial, directo e indexado (`clock_gettime`).
- `src/bloque3_directorios.c`: recorrido de arbol con `opendir`/`readdir`, enlaces duros (`link`), simbolicos (`symlink`) y enlace roto.
- `src/bloque4_montaje_permisos.c`: inspeccion de `/proc/mounts` y `statvfs`, bloqueos `fcntl` entre procesos (`fork`) y permisos POSIX (`chmod`).
- `src/bloque5_vfs_sim.c`: simulacion estructural de superbloque, dentry, inodo, tabla global de archivos abiertos y descriptores por proceso.
- `main.c`: despachador CLI secuencial o por bloque (`--bloque 1..5`).
- `pruebas.c`: bateria de pruebas automatizadas sobre los cinco modulos.

## Compilacion en Linux / WSL

Compilar el programa principal y la suite de pruebas directamente con `gcc`:

    gcc -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
        main.c src/bloque*.c -o fs_demo

    gcc -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L \
        pruebas.c src/bloque*.c -o pruebas

## Ejecucion

Ejecutar todos los bloques en secuencia:

    ./fs_demo

Ejecutar un bloque especifico (del 1 al 5):

    ./fs_demo --bloque 1

Ejecutar las pruebas de verificacion:

    ./pruebas
