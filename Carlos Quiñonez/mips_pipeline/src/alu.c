#include "mips_sim.h"

/* alu_execute: valor_rs, operando_B, ALUOp -> resultado[31:0]
 * Ejecuta la operación aritmética/lógica indicada por ALUOp. */
int32_t alu_execute(int32_t a, int32_t b, alu_op_t op) {
    switch (op) {
        case ALU_ADD: return a + b;
        case ALU_SUB: return a - b;
        case ALU_AND: return a & b;
        case ALU_OR:  return a | b;
        case ALU_NOR: return ~(a | b);
        case ALU_XOR: return a ^ b;
        case ALU_NOP:
        default:      return 0;
    }
}

/* zero_flag: resultado -> zero (1 bit)
 * Se usa para resolver beq/bne: si resultado == 0, la resta indica
 * que los dos registros eran iguales. */
int zero_flag(int32_t result) {
    return result == 0;
}

/* alu(): función principal de la etapa Execute.
 * El segundo operando es val_rt o el inmediato con signo extendido,
 * según la señal de control ALUSrc. */
alu_out_t alu(const decode_out_t *d) {
    alu_out_t out;
    int32_t operand_b = d->control.ALUSrc ? d->imm32 : d->val_rt;

    out.result = alu_execute(d->val_rs, operand_b, d->control.ALUOp);
    out.zero   = zero_flag(out.result);
    return out;
}
