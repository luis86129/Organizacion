/* =====================================================================
 * stage_id.c -- ETAPA 2: Instruction Decode
 *
 * Subfunciones (segun el diagrama del Proyecto Parte 1):
 *   splitFields  -> parte la instruccion en op/rs/rt/rd/shamt/funct/imm
 *   controlUnit  -> genera las senales de control a partir del opcode
 *   readRegs     -> lee $rs y $rt del banco de registros
 *   signExtend   -> extiende el inmediato de 16 a 32 bits CON SIGNO
 *   calcTargets  -> destino de beq/bne y destino de j
 *
 * OJO: en el mismo ciclo, WB ya escribio el banco antes de que ID lea
 * (el pipeline llama a las etapas en orden inverso). Por eso una
 * dependencia a distancia 3 no necesita burbuja.
 * ===================================================================== */
#include <string.h>
#include "mips.h"

/* --------------------------------------------------------------------
 * splitFields: indices exactos segun el formato MIPS.
 * -------------------------------------------------------------------- */
Fields id_split_fields(uint32_t instr)
{
    Fields f;

    f.op     = (uint8_t) ((instr >> 26) & 0x3Fu);  /* instr[31:26] */
    f.rs     = (uint8_t) ((instr >> 21) & 0x1Fu);  /* instr[25:21] */
    f.rt     = (uint8_t) ((instr >> 16) & 0x1Fu);  /* instr[20:16] */
    f.rd     = (uint8_t) ((instr >> 11) & 0x1Fu);  /* instr[15:11] */
    f.shamt  = (uint8_t) ((instr >> 6)  & 0x1Fu);  /* instr[10:6]  */
    f.funct  = (uint8_t) ( instr        & 0x3Fu);  /* instr[5:0]   */
    f.imm16  = (uint16_t)( instr        & 0xFFFFu);/* instr[15:0]  */
    f.addr26 = instr & 0x03FFFFFFu;                /* instr[25:0]  */
    return f;
}

/* --------------------------------------------------------------------
 * signExtend: la extension es CON SIGNO. Si se hiciera con ceros, un
 * lw $t0,-4($sp) apuntaria a una direccion enorme.
 * -------------------------------------------------------------------- */
uint32_t id_sign_extend(uint16_t imm16)
{
    return (uint32_t)(int32_t)(int16_t)imm16;
}

/* --------------------------------------------------------------------
 * controlUnit: una fila por instruccion de la ISA.
 * Devuelve 0 si el opcode es valido, -1 si es ilegal (se vuelve NOP).
 * -------------------------------------------------------------------- */
int id_control_unit(uint8_t op, uint8_t funct, Control *out)
{
    if (out == NULL) {
        return -1;
    }
    memset(out, 0, sizeof(*out));

    if (op == OP_RTYPE) {
        if (funct == F_JR) {
            /* jr usa $rs (no $rd) y no escribe ningun registro. */
            out->jump_reg = 1u;
            return 0;
        }
        if (funct == F_ADD || funct == F_SUB || funct == F_AND ||
            funct == F_OR  || funct == F_XOR || funct == F_NOR) {
            out->reg_dst   = 1u;          /* destino rd            */
            out->alu_src   = 0u;          /* segundo operando = rt */
            out->alu_op    = ALUOP_FUNCT; /* la ALU mira el funct  */
            out->reg_write = 1u;
            return 0;
        }
        return -1;  /* funct no soportado */
    }

    if (op == OP_ADDI) {
        out->alu_src   = 1u;
        out->alu_op    = ALUOP_ADD;
        out->reg_write = 1u;   /* destino rt (reg_dst = 0) */
        return 0;
    }
    if (op == OP_LW) {
        out->alu_src    = 1u;
        out->alu_op     = ALUOP_ADD;
        out->mem_read   = 1u;
        out->mem_to_reg = 1u;
        out->reg_write  = 1u;
        return 0;
    }
    if (op == OP_SW) {
        out->alu_src   = 1u;
        out->alu_op    = ALUOP_ADD;
        out->mem_write = 1u;   /* no escribe registros */
        return 0;
    }
    if (op == OP_BEQ) {
        out->alu_op = ALUOP_SUB;  /* la resta genera el flag zero */
        out->branch = 1u;
        return 0;
    }
    if (op == OP_BNE) {
        out->alu_op    = ALUOP_SUB;
        out->branch    = 1u;
        out->branch_ne = 1u;      /* se toma cuando zero = 0 */
        return 0;
    }
    if (op == OP_J) {
        out->jump = 1u;
        return 0;
    }

    return -1;  /* opcode desconocido */
}

/* readRegs: $0 siempre lee cero. */
static uint32_t read_reg(const RegFile *rf, uint8_t idx)
{
    if (idx == 0u) {
        return 0u;
    }
    return rf->r[idx & 31u];
}

/* calcTargets: el desplazamiento del branch es relativo a PC+4 y va
 * multiplicado por 4; el destino de j toma los 4 bits altos de PC+4. */
static void calc_targets(uint32_t pc_plus4, uint32_t imm32, uint32_t addr26,
                         uint32_t *branch_target, uint32_t *jump_target)
{
    *branch_target = pc_plus4 + (imm32 << 2);
    *jump_target   = (pc_plus4 & 0xF0000000u) | ((addr26 << 2) & 0x0FFFFFFCu);
}

IDOut stage_id(const IFID *in, const RegFile *rf)
{
    IDOut  out;
    Fields f;

    /* Validaciones previas antes de empezar el algoritmo. */
    memset(&out, 0, sizeof(out));
    if (in == NULL || rf == NULL) {
        out.error = MIPS_ERR_NULL;
        return out;
    }

    out.error = MIPS_OK;
    if (in->valid == 0) {
        return out;   /* burbuja entra, burbuja sale */
    }

    f = id_split_fields(in->instr);

    if (id_control_unit(f.op, f.funct, &out.idex.c) != 0) {
        /* Instruccion ilegal: se propaga como NOP, nunca como un add. */
        memset(&out.idex, 0, sizeof(out.idex));
        out.error = MIPS_ERR_ILLEGAL;
        return out;
    }

    out.idex.valid    = 1;
    out.idex.rs       = f.rs;
    out.idex.rt       = f.rt;
    out.idex.rd       = f.rd;
    out.idex.funct    = f.funct;
    out.idex.pc_plus4 = in->pc_plus4;
    out.idex.rs_val   = read_reg(rf, f.rs);
    out.idex.rt_val   = read_reg(rf, f.rt);
    out.idex.imm32    = id_sign_extend(f.imm16);

    calc_targets(in->pc_plus4, out.idex.imm32, f.addr26,
                 &out.idex.branch_target, &out.idex.jump_target);

    return out;
}
