/* =====================================================================
 * programs.h -- Vectores de prueba del test integral (S1..S8).
 * Los mismos programas los usa tools/vecgen para generar los .txt.
 * ===================================================================== */
#ifndef PROGRAMS_H
#define PROGRAMS_H

#include <stdint.h>

#define N_PROGRAMS 8
#define PROG_MAX_WORDS 64

typedef struct {
    const char *id;                    /* "s1", "s2", ...            */
    const char *descripcion;
    uint32_t    code[PROG_MAX_WORDS];  /* memoria de instrucciones   */
    uint32_t    n_code;
    uint32_t    data[PROG_MAX_WORDS];  /* memoria de datos inicial   */
    uint32_t    n_data;
} Program;

/* Devuelve el programa numero i (0..N_PROGRAMS-1). */
Program prog_get(int i);

#endif /* PROGRAMS_H */
