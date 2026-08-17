/* =====================================================================
 * stage_if.c -- ETAPA 1: Instruction Fetch + Program Counter
 *
 * Subfunciones (segun el diagrama del Proyecto Parte 1):
 *   selectNextPC  -> elige entre PC+4 y el destino del salto
 *   checkAlign    -> el PC debe ser multiplo de 4
 *   fetchInstr    -> lee la palabra de la memoria de instrucciones
 *   incrementPC   -> PC + 4
 *
 * Entradas : pc[31:0], taken, target[31:0], memoria de instrucciones
 * Salidas  : instr[31:0], pcPlus4[31:0], nextPC[31:0], error
 * ===================================================================== */
#include <string.h>
#include "mips.h"

/* El PC debe caer en frontera de palabra. Devuelve 1 si esta alineado. */
static int check_align(uint32_t pc)
{
    return (pc % 4u) == 0u;
}

/* PC + 4 */
static uint32_t increment_pc(uint32_t pc)
{
    return pc + 4u;
}

/* Mux del PC: si EX resolvio un salto tomado, el siguiente PC es el
 * destino; si no, es PC+4. */
static uint32_t select_next_pc(uint32_t pc_plus4, int taken, uint32_t target)
{
    if (taken != 0) {
        return target;
    }
    return pc_plus4;
}

/* Lee imem[pc/4]. Devuelve 1 si la direccion existe, 0 si se paso del
 * final del programa (eso no es error: simplemente ya no hay que buscar). */
static int fetch_instr(const InstrMem *im, uint32_t pc, uint32_t *out)
{
    uint32_t index = pc / 4u;

    if (index >= im->n_words) {
        *out = 0u;
        return 0;
    }
    *out = im->w[index];
    return 1;
}

IFOut stage_if(const IFIn *in, const InstrMem *im)
{
    IFOut    out;
    uint32_t instr = 0u;
    int      hay_instr;

    /* Validaciones previas antes de empezar el algoritmo. */
    memset(&out, 0, sizeof(out));
    if (in == NULL || im == NULL) {
        out.error = MIPS_ERR_NULL;
        return out;
    }

    out.error        = MIPS_OK;
    out.ifid.valid   = 0;
    out.ifid.instr   = 0u;
    out.ifid.pc_plus4 = increment_pc(in->pc);
    out.next_pc      = select_next_pc(out.ifid.pc_plus4, in->taken, in->target);

    if (!check_align(in->pc)) {
        /* PC corrupto: no se busca nada y se detiene el avance. */
        out.error   = MIPS_ERR_ALIGN;
        out.next_pc = in->pc;
        return out;
    }

    hay_instr = fetch_instr(im, in->pc, &instr);
    if (hay_instr) {
        out.ifid.valid = 1;
        out.ifid.instr = instr;
    }

    return out;
}
