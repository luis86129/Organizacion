/* =====================================================================
 * main.c -- Programa principal del simulador.
 *
 * Uso:
 *   ./mips_sim --imem vectors/s1_imem.txt
 *              [--dmem archivo.txt] [--regs archivo.txt]
 *              [--out-dmem archivo.txt] [--out-regs archivo.txt]
 *              [--max-cycles N] [--trace]
 *
 * Los tres .txt de entrada contienen una palabra de 32 bits por linea, escrita en binario (32 caracteres 0/1).
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mips.h"

static void uso(const char *prog)
{
    printf("uso: %s --imem <archivo.txt> [opciones]\n", prog);
    printf("  --dmem <f>       memoria de datos inicial\n");
    printf("  --regs <f>       banco de registros inicial (32 palabras)\n");
    printf("  --out-dmem <f>   vuelca la memoria de datos final\n");
    printf("  --out-regs <f>   vuelca el banco de registros final\n");
    printf("  --max-cycles <n> tope de ciclos (por defecto 100000)\n");
    printf("  --trace          imprime el estado ciclo por ciclo\n");
}

static void imprimir_registros(const RegFile *rf)
{
    unsigned i;

    printf("\nBanco de registros:\n");
    for (i = 0; i < NUM_REGS; i++) {
        printf("  $%-2u = 0x%08X (%11d)", i, rf->r[i], (int32_t)rf->r[i]);
        if ((i % 2u) == 1u) {
            printf("\n");
        }
    }
}

static void imprimir_traza(const Pipeline *p)
{
    printf("ciclo %-5llu PC=0x%08X | IF/ID %s | ID/EX %s | EX/MEM %s | MEM/WB %s\n",
           (unsigned long long)p->cycles, p->pc,
           p->ifid.valid  ? "ok" : "--",
           p->idex.valid  ? "ok" : "--",
           p->exmem.valid ? "ok" : "--",
           p->memwb.valid ? "ok" : "--");
}

int main(int argc, char **argv)
{
    InstrMem im;
    DataMem  dm;
    RegFile  rf;
    Pipeline p;

    const char *f_imem = NULL, *f_dmem = NULL, *f_regs = NULL;
    const char *o_dmem = NULL, *o_regs = NULL;
    uint64_t    max_cycles = 100000u;
    int         trace = 0;
    int         i, fin = 0;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--imem") == 0 && i + 1 < argc) {
            f_imem = argv[++i];
        } else if (strcmp(argv[i], "--dmem") == 0 && i + 1 < argc) {
            f_dmem = argv[++i];
        } else if (strcmp(argv[i], "--regs") == 0 && i + 1 < argc) {
            f_regs = argv[++i];
        } else if (strcmp(argv[i], "--out-dmem") == 0 && i + 1 < argc) {
            o_dmem = argv[++i];
        } else if (strcmp(argv[i], "--out-regs") == 0 && i + 1 < argc) {
            o_regs = argv[++i];
        } else if (strcmp(argv[i], "--max-cycles") == 0 && i + 1 < argc) {
            max_cycles = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--trace") == 0) {
            trace = 1;
        } else {
            uso(argv[0]);
            return 1;
        }
    }

    if (f_imem == NULL) {
        uso(argv[0]);
        return 1;
    }

    mem_init_imem(&im);
    mem_init_dmem(&dm);
    regfile_init(&rf);

    if (mem_load_imem(&im, f_imem) != 0) {
        return 1;
    }
    if (f_dmem != NULL && mem_load_dmem(&dm, f_dmem) != 0) {
        return 1;
    }
    if (f_regs != NULL && regfile_load(&rf, f_regs) != 0) {
        return 1;
    }

    printf("Simulador MIPS con pipeline de 5 etapas\n");
    printf("  imem : %s (%u instrucciones)\n", f_imem, im.n_words);
    printf("  dmem : %s (%u palabras)\n",
           (f_dmem != NULL) ? f_dmem : "vacia", dm.n_words);

    pipeline_init(&p, 0u);

    while (fin == 0 && p.cycles < max_cycles) {
        fin = pipeline_step(&p, &im, &dm, &rf);
        if (trace) {
            imprimir_traza(&p);
        }
    }

    printf("\nResultado: %s\n", mips_error_str(p.error));
    printf("  ciclos             : %llu\n", (unsigned long long)p.cycles);
    printf("  instrucciones       : %llu\n", (unsigned long long)p.instructions);
    printf("  burbujas (stalls)   : %llu\n", (unsigned long long)p.stalls);
    printf("  descartes (flushes) : %llu\n", (unsigned long long)p.flushes);

    imprimir_registros(&rf);

    if (o_regs != NULL && mem_dump_words(rf.r, NUM_REGS, o_regs) == 0) {
        printf("\nbanco de registros volcado en %s\n", o_regs);
    }
    if (o_dmem != NULL && mem_dump_words(dm.w, dm.n_words, o_dmem) == 0) {
        printf("memoria de datos volcada en %s\n", o_dmem);
    }

    return (p.error == MIPS_OK) ? 0 : 1;
}
