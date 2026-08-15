#include "mips.h"

#include <stdio.h>
#include <stdlib.h>


/*
 * Uso:
 *
 * ./mips_sim programa.txt memoria.txt registros.txt
 */

int main(
    int argc,
    char *argv[]
) {

    /*
     * Verificar argumentos.
     */

    if (
        argc < 4 ||
        argc > 5
    ) {

        printf(
            "Uso:\n"
            "%s programa.txt memoria.txt registros.txt [ciclos]\n",
            argv[0]
        );

        return 1;
    }


    /* Crear procesador */

    MIPS cpu = {0};


    /*
     * Cantidad máxima de ciclos.
     *
     * Si no se especifica:
     * 100 ciclos.
     */

    size_t max_cycles = 100;


    if (argc == 5) {

        max_cycles =
            (size_t)
            strtoul(
                argv[4],
                NULL,
                10
            );
    }


    /* =====================================================
       CARGAR PROGRAMA
       ===================================================== */

    if (
        load_binary_lines(
            argv[1],
            cpu.program,
            MAX_PROGRAM,
            &cpu.programSize
        ) != 0
    ) {

        printf(
            "Error leyendo programa: %s\n",
            argv[1]
        );

        return 1;
    }


    /* =====================================================
       CARGAR MEMORIA
       ===================================================== */

    if (
        load_memory(
            argv[2],
            &cpu.memory
        ) != 0
    ) {

        printf(
            "Error leyendo memoria: %s\n",
            argv[2]
        );

        return 1;
    }


    /* =====================================================
       CARGAR REGISTROS
       ===================================================== */

    if (
        load_registers(
            argv[3],
            &cpu.rf
        ) != 0
    ) {

        printf(
            "Error leyendo registros: %s\n",
            argv[3]
        );

        return 1;
    }


    /*
     * PC inicial.
     */

    cpu.pc = 0;


    printf(
        "\n====================================\n"
        "       SIMULADOR MIPS 32 BITS\n"
        "====================================\n"
    );


    printf(
        "Programa       : %s\n",
        argv[1]
    );


    printf(
        "Memoria        : %s\n",
        argv[2]
    );


    printf(
        "Registros      : %s\n",
        argv[3]
    );


    printf(
        "Instrucciones  : %zu\n",
        cpu.programSize
    );


    printf(
        "Memoria cargada: %zu palabras\n",
        cpu.memory.count
    );


    /* =====================================================
       EJECUTAR
       ===================================================== */

    int result =
        execute_program(
            &cpu,
            max_cycles
        );


    /* Mostrar registros */

    print_registers(
        &cpu.rf
    );


    printf(
        "\nPC final = %u\n",
        cpu.pc
    );


    if (result == 0) {

        printf(
            "\nSimulacion terminada correctamente.\n"
        );

        return 0;
    }


    printf(
        "\nLa simulacion termino con errores.\n"
    );


    return 1;
}