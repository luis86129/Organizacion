/* =====================================================================
 * encode.h -- Ensamblador minimo: arma las palabras de 32 bits de cada
 *             instruccion. Lo usan los tests y el generador de vectores
 *             para no escribir opcodes en hexadecimal a mano.
 * ===================================================================== */
#ifndef ENCODE_H
#define ENCODE_H

#include <stdint.h>
#include "mips.h"

/* Formato R: op(6) rs(5) rt(5) rd(5) shamt(5) funct(6) */
static inline uint32_t enc_r(uint8_t rs, uint8_t rt, uint8_t rd, uint8_t funct)
{
    return ((uint32_t)OP_RTYPE << 26) | ((uint32_t)(rs & 31u) << 21) |
           ((uint32_t)(rt & 31u) << 16) | ((uint32_t)(rd & 31u) << 11) |
           (uint32_t)(funct & 63u);
}

/* Formato I: op(6) rs(5) rt(5) imm(16) */
static inline uint32_t enc_i(uint8_t op, uint8_t rs, uint8_t rt, int16_t imm)
{
    return ((uint32_t)(op & 63u) << 26) | ((uint32_t)(rs & 31u) << 21) |
           ((uint32_t)(rt & 31u) << 16) | ((uint32_t)(uint16_t)imm);
}

/* Formato J: op(6) addr(26). addr es el destino en palabras. */
static inline uint32_t enc_j(uint8_t op, uint32_t addr26)
{
    return ((uint32_t)(op & 63u) << 26) | (addr26 & 0x03FFFFFFu);
}

/* Atajos legibles para armar programas de prueba. */
#define ASM_ADD(d,s,t)   enc_r((s),(t),(d),F_ADD)
#define ASM_SUB(d,s,t)   enc_r((s),(t),(d),F_SUB)
#define ASM_AND(d,s,t)   enc_r((s),(t),(d),F_AND)
#define ASM_OR(d,s,t)    enc_r((s),(t),(d),F_OR)
#define ASM_XOR(d,s,t)   enc_r((s),(t),(d),F_XOR)
#define ASM_NOR(d,s,t)   enc_r((s),(t),(d),F_NOR)
#define ASM_JR(s)        enc_r((s),0,0,F_JR)
#define ASM_ADDI(t,s,i)  enc_i(OP_ADDI,(s),(t),(i))
#define ASM_LW(t,i,s)    enc_i(OP_LW,(s),(t),(i))
#define ASM_SW(t,i,s)    enc_i(OP_SW,(s),(t),(i))
#define ASM_BEQ(s,t,i)   enc_i(OP_BEQ,(s),(t),(i))
#define ASM_BNE(s,t,i)   enc_i(OP_BNE,(s),(t),(i))
#define ASM_J(a)         enc_j(OP_J,(a))
#define ASM_NOP()        enc_r(0,0,0,F_ADD)   /* add $0,$0,$0 */

#endif /* ENCODE_H */
