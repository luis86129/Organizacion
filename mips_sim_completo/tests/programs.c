/* =====================================================================
 * programs.c -- Los ocho vectores de prueba del plan de verificacion
 *               definido en el Proyecto Parte 1 (rol Critico).
 *
 * Cada programa trae su codigo y, si lo necesita, el contenido inicial
 * de la memoria de datos. Los resultados esperados se comprueban en
 * tests/test_integration.c.
 * ===================================================================== */
#include <string.h>
#include "programs.h"
#include "encode.h"

static void set_code(Program *p, const uint32_t *code, uint32_t n)
{
    memcpy(p->code, code, n * sizeof(uint32_t));
    p->n_code = n;
}

static void set_data(Program *p, const uint32_t *data, uint32_t n)
{
    memcpy(p->data, data, n * sizeof(uint32_t));
    p->n_data = n;
}

Program prog_get(int i)
{
    Program p;

    memset(&p, 0, sizeof(p));

    if (i == 0) {
        /* S1: camino aritmetico. $3 = 13, $4 = 7 */
        uint32_t c[] = {
            ASM_ADDI(1, 0, 10),
            ASM_ADDI(2, 0, 3),
            ASM_ADD(3, 1, 2),
            ASM_SUB(4, 1, 2)
        };
        p.id = "s1";
        p.descripcion = "aritmetica basica (add, sub, addi)";
        set_code(&p, c, 4);
    } else if (i == 1) {
        /* S2: las cuatro logicas sobre 0x00FF y 0x000F */
        uint32_t c[] = {
            ASM_ADDI(1, 0, 0x00FF),
            ASM_ADDI(2, 0, 0x000F),
            ASM_AND(3, 1, 2),   /* 0x0000000F */
            ASM_OR(4, 1, 2),    /* 0x000000FF */
            ASM_XOR(5, 1, 2),   /* 0x000000F0 */
            ASM_NOR(6, 1, 2)    /* 0xFFFFFF00 */
        };
        p.id = "s2";
        p.descripcion = "operaciones logicas (and, or, xor, nor)";
        set_code(&p, c, 6);
    } else if (i == 2) {
        /* S3: sw seguido de lw a la misma direccion. $3 = 42 */
        uint32_t c[] = {
            ASM_ADDI(1, 0, 42),
            ASM_ADDI(2, 0, 16),
            ASM_SW(1, 0, 2),
            ASM_LW(3, 0, 2)
        };
        p.id = "s3";
        p.descripcion = "ida y vuelta a memoria (sw + lw)";
        set_code(&p, c, 4);
    } else if (i == 3) {
        /* S4: riesgo load-use. El add usa $3 justo despues del lw. */
        uint32_t c[] = {
            ASM_ADDI(2, 0, 16),
            ASM_ADDI(1, 0, 7),
            ASM_SW(1, 0, 2),
            ASM_LW(3, 0, 2),
            ASM_ADD(4, 3, 3)    /* $4 = 14 */
        };
        p.id = "s4";
        p.descripcion = "riesgo load-use (lw seguido de uso inmediato)";
        set_code(&p, c, 5);
    } else if (i == 4) {
        /* S5: beq tomado y beq no tomado.
         * $3 y $4 deben quedar en 0, $5 = 7, $6 = 3 */
        uint32_t c[] = {
            ASM_ADDI(1, 0, 5),
            ASM_ADDI(2, 0, 5),
            ASM_BEQ(1, 2, 2),    /* iguales: salta a la instruccion 5 */
            ASM_ADDI(3, 0, 99),  /* no debe ejecutarse */
            ASM_ADDI(4, 0, 99),  /* no debe ejecutarse */
            ASM_ADDI(5, 0, 7),
            ASM_BEQ(1, 0, 1),    /* 5 != 0: no salta */
            ASM_ADDI(6, 0, 3)
        };
        p.id = "s5";
        p.descripcion = "beq tomado y no tomado, con flush";
        set_code(&p, c, 8);
    } else if (i == 5) {
        /* S6: bne con operandos distintos e iguales.
         * $3 = 0, $4 = 1, $5 = 2 */
        uint32_t c[] = {
            ASM_ADDI(1, 0, 4),
            ASM_ADDI(2, 0, 9),
            ASM_BNE(1, 2, 1),    /* distintos: salta */
            ASM_ADDI(3, 0, 99),  /* no debe ejecutarse */
            ASM_ADDI(4, 0, 1),
            ASM_BNE(4, 4, 1),    /* iguales: no salta */
            ASM_ADDI(5, 0, 2)
        };
        p.id = "s6";
        p.descripcion = "bne en sus dos casos";
        set_code(&p, c, 7);
    } else if (i == 6) {
        /* S7: j y jr. $2 y $3 quedan en 0, $4 = 7 */
        uint32_t c[] = {
            ASM_ADDI(1, 0, 20),  /* direccion de retorno (palabra 5) */
            ASM_J(4),            /* salta a la palabra 4             */
            ASM_ADDI(2, 0, 99),  /* no debe ejecutarse               */
            ASM_ADDI(3, 0, 99),  /* no debe ejecutarse               */
            ASM_JR(1),           /* vuelve a la direccion 20         */
            ASM_ADDI(4, 0, 7)
        };
        p.id = "s7";
        p.descripcion = "salto absoluto (j) y retorno por registro (jr)";
        set_code(&p, c, 6);
    } else {
        /* S8: bucle que suma un arreglo de 4 palabras y guarda el
         * total en la direccion 32. $1 = 100, dmem[8] = 100 */
        uint32_t c[] = {
            ASM_ADDI(1, 0, 0),   /* suma      */
            ASM_ADDI(2, 0, 0),   /* puntero   */
            ASM_ADDI(3, 0, 16),  /* limite    */
            ASM_LW(4, 0, 2),     /* loop:     */
            ASM_ADD(1, 1, 4),
            ASM_ADDI(2, 2, 4),
            ASM_BNE(2, 3, -4),   /* vuelve a loop */
            ASM_SW(1, 32, 0)
        };
        uint32_t d[] = { 10u, 20u, 30u, 40u };
        p.id = "s8";
        p.descripcion = "bucle contador y suma de un arreglo";
        set_code(&p, c, 8);
        set_data(&p, d, 4);
    }

    return p;
}
