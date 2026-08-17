/* =====================================================================
 * stage_ex.c -- ETAPA 3: ALU (Execute)
 *
 * Subfunciones (segun el diagrama del Proyecto Parte 1):
 *   aluControl   -> traduce aluOp[1:0] + funct[5:0] a aluControl[3:0]
 *   selectOperB  -> mux: segundo operando = $rt o el inmediato
 *   execute      -> la operacion propiamente dicha
 *   zeroFlag     -> flag zero (resultado == 0)
 *   branchTaken  -> decide beq / bne / j / jr y calcula el destino
 *   selectDest   -> mux regDst: destino rd (tipo R) o rt (tipo I)
 *
 * Todos los saltos se resuelven aqui: beq y bne reutilizan la resta de
 * la ALU, asi no hace falta un comparador aparte. El precio son dos
 * ciclos de penalidad, que el pipeline paga con flush.
 * ===================================================================== */
#include <string.h>
#include "mips.h"

/* aluControl[3:0] a partir de aluOp y funct. -1 si el funct es ilegal. */
int ex_alu_control(uint8_t alu_op, uint8_t funct)
{
    if (alu_op == ALUOP_ADD) {
        return (int)ALUC_ADD;   /* addi, lw, sw */
    }
    if (alu_op == ALUOP_SUB) {
        return (int)ALUC_SUB;   /* beq, bne     */
    }
    if (alu_op == ALUOP_FUNCT) {
        switch (funct) {
            case F_ADD: return (int)ALUC_ADD;
            case F_SUB: return (int)ALUC_SUB;
            case F_AND: return (int)ALUC_AND;
            case F_OR:  return (int)ALUC_OR;
            case F_XOR: return (int)ALUC_XOR;
            case F_NOR: return (int)ALUC_NOR;
            default:    return -1;
        }
    }
    return (int)ALUC_ADD;  /* j y jr no usan la ALU */
}

/* La operacion. Se trabaja en uint32_t: el desbordamiento envuelve, que
 * es exactamente lo que hace addu/subu. add con overflow no lanza
 * excepcion en este simulador (restriccion declarada en la Parte 1). */
uint32_t ex_execute(uint32_t a, uint32_t b, uint8_t alu_ctrl)
{
    switch (alu_ctrl) {
        case ALUC_AND: return a & b;
        case ALUC_OR:  return a | b;
        case ALUC_ADD: return a + b;
        case ALUC_XOR: return a ^ b;
        case ALUC_SUB: return a - b;
        case ALUC_NOR: return ~(a | b);
        default:       return 0u;
    }
}

/* Mux del segundo operando. */
static uint32_t select_oper_b(const IDEX *in)
{
    if (in->c.alu_src != 0u) {
        return in->imm32;
    }
    return in->rt_val;
}

static int zero_flag(uint32_t result)
{
    return result == 0u;
}

/* Mux regDst: tipo R escribe rd, tipo I escribe rt. */
static uint8_t select_dest(const IDEX *in)
{
    if (in->c.reg_dst != 0u) {
        return in->rd;
    }
    return in->rt;
}

/* Decide si hay transferencia de control y hacia donde. */
static int branch_taken(const IDEX *in, int zero, uint32_t *target)
{
    if (in->c.branch != 0u) {
        int tomar = (in->c.branch_ne != 0u) ? (zero == 0) : (zero != 0);
        if (tomar) {
            *target = in->branch_target;
            return 1;
        }
        return 0;
    }
    if (in->c.jump != 0u) {
        *target = in->jump_target;
        return 1;
    }
    if (in->c.jump_reg != 0u) {
        *target = in->rs_val;   /* jr usa $rs */
        return 1;
    }
    return 0;
}

EXOut stage_ex(const IDEX *in)
{
    EXOut    out;
    int      alu_ctrl;
    uint32_t b;

    /* Validaciones previas antes de empezar el algoritmo. */
    memset(&out, 0, sizeof(out));
    if (in == NULL) {
        out.error = MIPS_ERR_NULL;
        return out;
    }

    out.error = MIPS_OK;
    if (in->valid == 0) {
        return out;   /* burbuja */
    }

    alu_ctrl = ex_alu_control(in->c.alu_op, in->funct);
    if (alu_ctrl < 0) {
        out.error = MIPS_ERR_ILLEGAL;
        return out;
    }

    b = select_oper_b(in);

    out.exmem.valid        = 1;
    out.exmem.c            = in->c;
    out.exmem.alu_result   = ex_execute(in->rs_val, b, (uint8_t)alu_ctrl);
    out.exmem.rt_val       = in->rt_val;
    out.exmem.write_reg    = select_dest(in);
    out.exmem.zero         = zero_flag(out.exmem.alu_result);
    out.exmem.branch_taken = branch_taken(in, out.exmem.zero,
                                          &out.exmem.branch_pc);
    return out;
}
