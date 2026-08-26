/* =====================================================================
 * stage_wb.c -- ETAPA 5: Write Back
 *
 * Subfunciones (segun el diagrama del Proyecto Parte 1):
 *   selectData   -> mux memToReg: dato de memoria o resultado de la ALU
 *   selectDest   -> registro destino que viene propagado desde EX
 *   writeRegFile -> escritura real, con la guarda de $0
 *
 * La guarda de $0 es obligatoria: sin ella, un addi $0,$0,5 dejaria el
 * registro cero contaminado y romperia todo el programa siguiente.
 * ===================================================================== */
#include <string.h>
#include "mips.h"

/* Mux memToReg. */
static uint32_t select_data(const MEMWB *in)
{
    if (in->c.mem_to_reg != 0u) {
        return in->read_data;
    }
    return in->alu_result;
}

static uint8_t select_dest(const MEMWB *in)
{
    return (uint8_t)(in->write_reg & 31u);
}

/* Escritura efectiva: nunca se escribe $0. */
static int write_regfile(RegFile *rf, uint8_t reg, uint32_t value)
{
    if (reg == 0u) {
        return 0;
    }
    rf->r[reg] = value;
    return 1;
}

WBOut stage_wb(const MEMWB *in, RegFile *rf)
{
    WBOut out;

    /* Validaciones previas antes de empezar el algoritmo. */
    memset(&out, 0, sizeof(out));
    if (in == NULL || rf == NULL) {
        return out;
    }
    if (in->valid == 0 || in->c.reg_write == 0u) {
        return out;   /* burbuja, o instruccion que no escribe (sw, beq, j) */
    }

    out.reg   = select_dest(in);
    out.value = select_data(in);
    out.wrote = write_regfile(rf, out.reg, out.value);

    return out;
}
