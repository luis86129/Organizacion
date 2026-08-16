#ifndef MINI_ASM_H
#define MINI_ASM_H

#include <stdint.h>
#include "mips_sim.h"

/* Ensamblador mínimo: arma la palabra de 32 bits de una instrucción a
 * partir de sus campos, para poder escribir programas de prueba legibles
 * en lugar de escribir hex a mano. Uso puramente de testing, no es parte
 * del simulador en sí. */

static inline uint32_t asm_r(uint32_t opcode, uint32_t rs, uint32_t rt,
                              uint32_t rd, uint32_t shamt, uint32_t funct) {
    return (opcode << 26) | (rs << 21) | (rt << 16) | (rd << 11) |
           (shamt << 6) | funct;
}

static inline uint32_t asm_i(uint32_t opcode, uint32_t rs, uint32_t rt,
                              uint16_t imm16) {
    return (opcode << 26) | (rs << 21) | (rt << 16) | imm16;
}

static inline uint32_t asm_j(uint32_t opcode, uint32_t addr26) {
    return (opcode << 26) | (addr26 & 0x3FFFFFF);
}

#define ASM_ADD(rd, rs, rt)  asm_r(OP_RTYPE, rs, rt, rd, 0, FN_ADD)
#define ASM_SUB(rd, rs, rt)  asm_r(OP_RTYPE, rs, rt, rd, 0, FN_SUB)
#define ASM_AND(rd, rs, rt)  asm_r(OP_RTYPE, rs, rt, rd, 0, FN_AND)
#define ASM_OR(rd, rs, rt)   asm_r(OP_RTYPE, rs, rt, rd, 0, FN_OR)
#define ASM_NOR(rd, rs, rt)  asm_r(OP_RTYPE, rs, rt, rd, 0, FN_NOR)
#define ASM_XOR(rd, rs, rt)  asm_r(OP_RTYPE, rs, rt, rd, 0, FN_XOR)
#define ASM_JR(rs)           asm_r(OP_RTYPE, rs, 0, 0, 0, FN_JR)

#define ASM_ADDI(rt, rs, imm) asm_i(OP_ADDI, rs, rt, (uint16_t)(imm))
#define ASM_LW(rt, rs, imm)   asm_i(OP_LW,   rs, rt, (uint16_t)(imm))
#define ASM_SW(rt, rs, imm)   asm_i(OP_SW,   rs, rt, (uint16_t)(imm))
#define ASM_BEQ(rs, rt, imm)  asm_i(OP_BEQ,  rs, rt, (uint16_t)(imm))
#define ASM_BNE(rs, rt, imm)  asm_i(OP_BNE,  rs, rt, (uint16_t)(imm))
#define ASM_J(addr)           asm_j(OP_J, addr)
#define ASM_HALT()            0x00000000u

#endif /* MINI_ASM_H */
