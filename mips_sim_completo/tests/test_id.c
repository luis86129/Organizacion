/* =====================================================================
 * test_id.c -- Unit test de la ETAPA 2 (Instruction Decode)
 *
 * Casos: separacion de campos, tabla de control instruccion por
 *        instruccion, extension de signo, lectura de registros,
 *        destinos de salto e instruccion ilegal.
 * ===================================================================== */
#include "minitest.h"
#include "mips.h"
#include "encode.h"

int main(void)
{
    Fields  f;
    Control c;
    RegFile rf;
    IFID    in;
    IDOut   out;

    MT_BEGIN("Etapa 2: Instruction Decode");

    /* --- splitFields sobre add $3,$1,$2 --- */
    f = id_split_fields(ASM_ADD(3, 1, 2));
    CHECK_EQ(f.op, OP_RTYPE, "op de tipo R");
    CHECK_EQ(f.rs, 1u, "campo rs");
    CHECK_EQ(f.rt, 2u, "campo rt");
    CHECK_EQ(f.rd, 3u, "campo rd");
    CHECK_EQ(f.funct, F_ADD, "campo funct");

    /* --- splitFields sobre lw $5,-8($4) --- */
    f = id_split_fields(ASM_LW(5, -8, 4));
    CHECK_EQ(f.op, OP_LW, "op de lw");
    CHECK_EQ(f.rs, 4u, "rs de lw es la base");
    CHECK_EQ(f.rt, 5u, "rt de lw es el destino");
    CHECK_EQ(f.imm16, 0xFFF8u, "inmediato de 16 bits sin extender");

    /* --- signExtend: con signo, no con ceros --- */
    CHECK_EQ(id_sign_extend(0xFFF8u), 0xFFFFFFF8u, "inmediato negativo");
    CHECK_EQ(id_sign_extend(0x7FFFu), 0x00007FFFu, "inmediato positivo");
    CHECK_EQ(id_sign_extend(0x8000u), 0xFFFF8000u, "limite negativo");

    /* --- controlUnit por instruccion --- */
    CHECK(id_control_unit(OP_RTYPE, F_ADD, &c) == 0, "add es valida");
    CHECK(c.reg_dst == 1u && c.reg_write == 1u && c.alu_src == 0u,
          "control de tipo R");
    CHECK_EQ(c.alu_op, ALUOP_FUNCT, "aluOp de tipo R");

    CHECK(id_control_unit(OP_ADDI, 0, &c) == 0, "addi es valida");
    CHECK(c.reg_dst == 0u && c.alu_src == 1u && c.reg_write == 1u,
          "control de addi");

    CHECK(id_control_unit(OP_LW, 0, &c) == 0, "lw es valida");
    CHECK(c.mem_read == 1u && c.mem_to_reg == 1u && c.reg_write == 1u,
          "control de lw");

    CHECK(id_control_unit(OP_SW, 0, &c) == 0, "sw es valida");
    CHECK(c.mem_write == 1u && c.reg_write == 0u, "sw no escribe registros");

    CHECK(id_control_unit(OP_BEQ, 0, &c) == 0, "beq es valida");
    CHECK(c.branch == 1u && c.branch_ne == 0u && c.reg_write == 0u,
          "control de beq");
    CHECK_EQ(c.alu_op, ALUOP_SUB, "beq usa la resta de la ALU");

    CHECK(id_control_unit(OP_BNE, 0, &c) == 0, "bne es valida");
    CHECK(c.branch == 1u && c.branch_ne == 1u, "control de bne");

    CHECK(id_control_unit(OP_J, 0, &c) == 0, "j es valida");
    CHECK(c.jump == 1u && c.reg_write == 0u, "control de j");

    CHECK(id_control_unit(OP_RTYPE, F_JR, &c) == 0, "jr es valida");
    CHECK(c.jump_reg == 1u && c.reg_write == 0u, "jr no escribe registros");

    CHECK(id_control_unit(0x3Fu, 0, &c) == -1, "opcode desconocido es ilegal");
    CHECK(id_control_unit(OP_RTYPE, 0x3Fu, &c) == -1, "funct desconocido es ilegal");

    /* --- readRegs: valores y guarda de $0 --- */
    regfile_init(&rf);
    rf.r[1] = 100u;
    rf.r[2] = 7u;
    in.valid = 1;
    in.instr = ASM_SUB(3, 1, 2);
    in.pc_plus4 = 0x100u;
    out = stage_id(&in, &rf);
    CHECK(out.error == MIPS_OK, "sub decodifica sin error");
    CHECK_EQ(out.idex.rs_val, 100u, "valor leido de $rs");
    CHECK_EQ(out.idex.rt_val, 7u, "valor leido de $rt");
    CHECK_EQ(out.idex.rd, 3u, "destino rd propagado");

    in.instr = ASM_ADD(5, 0, 1);
    out = stage_id(&in, &rf);
    CHECK_EQ(out.idex.rs_val, 0u, "$0 siempre lee cero");

    /* --- calcTargets: branch relativo a PC+4 y por 4 --- */
    in.instr = ASM_BEQ(1, 2, 3);
    in.pc_plus4 = 0x100u;
    out = stage_id(&in, &rf);
    CHECK_EQ(out.idex.branch_target, 0x10Cu, "destino de beq = PC+4 + imm*4");

    in.instr = ASM_BEQ(1, 2, -2);
    out = stage_id(&in, &rf);
    CHECK_EQ(out.idex.branch_target, 0x0F8u, "destino de beq hacia atras");

    /* --- calcTargets: destino de j --- */
    in.instr = ASM_J(4);
    out = stage_id(&in, &rf);
    CHECK_EQ(out.idex.jump_target, 16u, "destino de j = addr*4");

    /* --- instruccion ilegal se vuelve NOP, no un add fantasma --- */
    in.instr = 0xFC000000u;   /* opcode 0x3F */
    out = stage_id(&in, &rf);
    CHECK(out.error == MIPS_ERR_ILLEGAL, "opcode ilegal reportado");
    CHECK(out.idex.valid == 0, "instruccion ilegal no avanza");
    CHECK(out.idex.c.reg_write == 0u, "instruccion ilegal no escribe");

    /* --- burbuja entra, burbuja sale --- */
    in.valid = 0;
    out = stage_id(&in, &rf);
    CHECK(out.idex.valid == 0, "burbuja se propaga");
    CHECK(out.error == MIPS_OK, "burbuja no genera error");

    /* --- parametros nulos --- */
    out = stage_id(NULL, &rf);
    CHECK(out.error == MIPS_ERR_NULL, "entrada nula se reporta");

    return MT_END();
}
