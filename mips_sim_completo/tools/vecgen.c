/* =====================================================================
 * vecgen.c -- Genera los archivos .txt de los vectores de prueba.
 *
 * Para cada programa S1..S8 escribe en vectors/:
 *     <id>_imem.txt  memoria de instrucciones
 *     <id>_dmem.txt  memoria de datos inicial (si el programa la usa)
 *     <id>_regs.txt  banco de registros inicial (32 palabras en cero)
 *
 * Asi el simulador se puede correr sin recompilar nada:
 *     ./mips_sim --imem vectors/s8_imem.txt --dmem vectors/s8_dmem.txt
 * ===================================================================== */
#include <stdio.h>
#include <string.h>
#include "mips.h"
#include "programs.h"

int main(void)
{
    Program  pr;
    char     path[256];
    uint32_t regs[NUM_REGS];
    int      i;

    memset(regs, 0, sizeof(regs));

    for (i = 0; i < N_PROGRAMS; i++) {
        pr = prog_get(i);

        snprintf(path, sizeof(path), "vectors/%s_imem.txt", pr.id);
        if (mem_dump_words(pr.code, pr.n_code, path) != 0) {
            return 1;
        }
        printf("  %-24s %u instrucciones  (%s)\n", path, pr.n_code,
               pr.descripcion);

        if (pr.n_data > 0u) {
            snprintf(path, sizeof(path), "vectors/%s_dmem.txt", pr.id);
            if (mem_dump_words(pr.data, pr.n_data, path) != 0) {
                return 1;
            }
            printf("  %-24s %u palabras de datos\n", path, pr.n_data);
        }

        snprintf(path, sizeof(path), "vectors/%s_regs.txt", pr.id);
        if (mem_dump_words(regs, NUM_REGS, path) != 0) {
            return 1;
        }
    }

    return 0;
}
