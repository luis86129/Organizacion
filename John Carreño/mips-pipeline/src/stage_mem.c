/* =====================================================================
 * stage_mem.c -- ETAPA 4: Memory
 *
 * Subfunciones (segun el diagrama del Proyecto Parte 1):
 *   mapAddress  -> direccion en bytes -> indice de palabra (addr / 4)
 *   checkBounds -> la palabra debe existir dentro de la memoria
 *   readWord    -> lw
 *   writeWord   -> sw
 *
 * Si la instruccion no es lw ni sw, la etapa solo deja pasar el
 * resultado de la ALU hacia MEM/WB.
 * ===================================================================== */
#include <string.h>
#include "mips.h"

/* mapAddress: la direccion viene en bytes, la memoria se indexa por
 * palabras. Devuelve 1 si la direccion esta alineada. */
static int map_address(uint32_t address, uint32_t *index)
{
    if ((address % 4u) != 0u) {
        return 0;
    }
    *index = address / 4u;
    return 1;
}

static int check_bounds(uint32_t index)
{
    return index < DMEM_WORDS;
}

static uint32_t read_word(const DataMem *dm, uint32_t index)
{
    return dm->w[index];
}

/* writeWord ademas ajusta n_words para que el volcado a .bin incluya
 * las palabras nuevas que escribio el programa. */
static void write_word(DataMem *dm, uint32_t index, uint32_t value)
{
    dm->w[index] = value;
    if (index + 1u > dm->n_words) {
        dm->n_words = index + 1u;
    }
}

MEMOut stage_mem(const EXMEM *in, DataMem *dm)
{
    MEMOut   out;
    uint32_t index = 0u;

    /* Validaciones previas antes de empezar el algoritmo. */
    memset(&out, 0, sizeof(out));
    if (in == NULL || dm == NULL) {
        out.error = MIPS_ERR_NULL;
        return out;
    }

    out.error = MIPS_OK;
    if (in->valid == 0) {
        return out;   /* burbuja */
    }

    out.memwb.valid      = 1;
    out.memwb.c          = in->c;
    out.memwb.alu_result = in->alu_result;
    out.memwb.write_reg  = in->write_reg;
    out.memwb.read_data  = 0u;

    if (in->c.mem_read == 0u && in->c.mem_write == 0u) {
        return out;   /* la etapa no hace nada mas que dejar pasar */
    }

    if (!map_address(in->alu_result, &index)) {
        out.error = MIPS_ERR_ALIGN;
        return out;
    }
    if (!check_bounds(index)) {
        out.error = MIPS_ERR_RANGE;
        return out;
    }

    if (in->c.mem_read != 0u) {
        out.memwb.read_data = read_word(dm, index);
    }
    if (in->c.mem_write != 0u) {
        write_word(dm, index, in->rt_val);
    }

    return out;
}
