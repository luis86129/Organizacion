#include "mips_sim.h"

/* extract_fields: instr[31:0] -> opcode, rs, rt, rd, shamt, funct, imm16, addr26
 * Corta la instrucción de 32 bits en los campos definidos por el formato
 * MIPS (tipo R, tipo I, tipo J). No todos los campos aplican a toda
 * instrucción, pero se extraen todos y cada subfunción usa solo los que
 * necesita. */
fields_t extract_fields(uint32_t instr) {
    fields_t f;
    f.opcode = (instr >> 26) & 0x3F;   /* instr[31:26] - 6 bits */
    f.rs     = (instr >> 21) & 0x1F;   /* instr[25:21] - 5 bits */
    f.rt     = (instr >> 16) & 0x1F;   /* instr[20:16] - 5 bits */
    f.rd     = (instr >> 11) & 0x1F;   /* instr[15:11] - 5 bits */
    f.shamt  = (instr >> 6)  & 0x1F;   /* instr[10:6]  - 5 bits */
    f.funct  = instr & 0x3F;           /* instr[5:0]   - 6 bits */
    f.imm16  = instr & 0xFFFF;         /* instr[15:0]  - 16 bits */
    f.addr26 = instr & 0x3FFFFFF;      /* instr[25:0]  - 26 bits */
    return f;
}

/* sign_extend: imm16 -> imm32
 * Extiende el signo del inmediato de 16 a 32 bits, replicando el bit 15
 * en los 16 bits altos. */
int32_t sign_extend(uint16_t imm16) {
    return (int32_t)(int16_t)imm16;
}

/* register_read: rs, rt -> val_rs, val_rt
 * Lee los valores actuales del banco de registros para rs y rt. */
void register_read(const int32_t *regs, uint32_t rs, uint32_t rt,
                    int32_t *val_rs, int32_t *val_rt) {
    *val_rs = regs[rs];
    *val_rt = regs[rt];
}

/* control_unit: opcode, funct -> señales de control
 * Genera todas las señales que las etapas siguientes (ALU, memory,
 * write_back) necesitan para saber qué hacer con esta instrucción. */
control_t control_unit(uint32_t opcode, uint32_t funct) {
    control_t c = {0};
    c.ALUOp = ALU_NOP;

    switch (opcode) {
        case OP_RTYPE:
            c.RegDst   = 1;
            c.RegWrite = 1;
            switch (funct) {
                case FN_ADD: c.ALUOp = ALU_ADD; break;
                case FN_SUB: c.ALUOp = ALU_SUB; break;
                case FN_AND: c.ALUOp = ALU_AND; break;
                case FN_OR:  c.ALUOp = ALU_OR;  break;
                case FN_NOR: c.ALUOp = ALU_NOR; break;
                case FN_XOR: c.ALUOp = ALU_XOR; break;
                case FN_JR:
                    c.JumpReg  = 1;
                    c.RegWrite = 0;
                    c.RegDst   = 0;
                    break;
                default: break;
            }
            break;

        case OP_ADDI:
            c.ALUSrc   = 1;
            c.RegWrite = 1;
            c.ALUOp    = ALU_ADD;
            break;

        case OP_LW:
            c.ALUSrc   = 1;
            c.MemRead  = 1;
            c.MemToReg = 1;
            c.RegWrite = 1;
            c.ALUOp    = ALU_ADD;
            break;

        case OP_SW:
            c.ALUSrc   = 1;
            c.MemWrite = 1;
            c.ALUOp    = ALU_ADD;
            break;

        case OP_BEQ:
            c.Branch = 1;
            c.ALUOp  = ALU_SUB;
            break;

        case OP_BNE:
            c.BranchNE = 1;
            c.ALUOp    = ALU_SUB;
            break;

        case OP_J:
            c.Jump = 1;
            break;

        default:
            break;
    }
    return c;
}

/* decode(): función principal de la etapa Instruction Decode.
 * Combina extract_fields + control_unit + sign_extend + register_read. */
decode_out_t decode(uint32_t instr, uint32_t pc_plus4, const int32_t *regs) {
    decode_out_t d;
    d.fields    = extract_fields(instr);
    d.control   = control_unit(d.fields.opcode, d.fields.funct);
    d.imm32     = sign_extend(d.fields.imm16);
    d.pc_plus4  = pc_plus4;
    register_read(regs, d.fields.rs, d.fields.rt, &d.val_rs, &d.val_rt);
    return d;
}
