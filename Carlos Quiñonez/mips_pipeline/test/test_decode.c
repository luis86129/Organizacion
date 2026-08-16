#include "mips_sim.h"
#include "mini_asm.h"
#include "test_common.h"

static void test_extract_fields_rtype(void) {
    TEST_CASE("extract_fields: instruccion tipo R (add $3,$1,$2)");
    uint32_t instr = ASM_ADD(3, 1, 2);
    fields_t f = extract_fields(instr);
    CHECK(f.opcode == OP_RTYPE);
    CHECK(f.rs == 1);
    CHECK(f.rt == 2);
    CHECK(f.rd == 3);
    CHECK(f.shamt == 0);
    CHECK(f.funct == FN_ADD);
    TEST_END();
}

static void test_extract_fields_itype(void) {
    TEST_CASE("extract_fields: instruccion tipo I (addi $2,$1,100)");
    uint32_t instr = ASM_ADDI(2, 1, 100);
    fields_t f = extract_fields(instr);
    CHECK(f.opcode == OP_ADDI);
    CHECK(f.rs == 1);
    CHECK(f.rt == 2);
    CHECK(f.imm16 == 100);
    TEST_END();
}

static void test_extract_fields_jtype(void) {
    TEST_CASE("extract_fields: instruccion tipo J (j 0x40)");
    uint32_t instr = ASM_J(0x40);
    fields_t f = extract_fields(instr);
    CHECK(f.opcode == OP_J);
    CHECK(f.addr26 == 0x40);
    TEST_END();
}

static void test_sign_extend(void) {
    TEST_CASE("sign_extend: positivo y negativo");
    CHECK(sign_extend(5) == 5);
    CHECK(sign_extend(0xFFFF) == -1);          /* -1 en 16 bits */
    CHECK(sign_extend(0x8000) == -32768);       /* bit de signo prendido */
    TEST_END();
}

static void test_register_read(void) {
    TEST_CASE("register_read: lee valores correctos de rs y rt");
    int32_t regs[NUM_REGS] = {0};
    regs[5] = 111;
    regs[6] = 222;
    int32_t val_rs, val_rt;
    register_read(regs, 5, 6, &val_rs, &val_rt);
    CHECK(val_rs == 111);
    CHECK(val_rt == 222);
    TEST_END();
}

static void test_control_unit_rtype(void) {
    TEST_CASE("control_unit: add (tipo R) genera señales correctas");
    control_t c = control_unit(OP_RTYPE, FN_ADD);
    CHECK(c.RegDst == 1);
    CHECK(c.RegWrite == 1);
    CHECK(c.ALUSrc == 0);
    CHECK(c.ALUOp == ALU_ADD);
    TEST_END();
}

static void test_control_unit_lw_sw(void) {
    TEST_CASE("control_unit: lw y sw generan señales de memoria correctas");
    control_t lw = control_unit(OP_LW, 0);
    CHECK(lw.MemRead == 1);
    CHECK(lw.MemToReg == 1);
    CHECK(lw.RegWrite == 1);
    CHECK(lw.ALUSrc == 1);

    control_t sw = control_unit(OP_SW, 0);
    CHECK(sw.MemWrite == 1);
    CHECK(sw.RegWrite == 0);
    TEST_END();
}

static void test_control_unit_branch(void) {
    TEST_CASE("control_unit: beq y bne activan Branch/BranchNE");
    control_t beq = control_unit(OP_BEQ, 0);
    CHECK(beq.Branch == 1);
    CHECK(beq.ALUOp == ALU_SUB);

    control_t bne = control_unit(OP_BNE, 0);
    CHECK(bne.BranchNE == 1);
    TEST_END();
}

static void test_control_unit_jump(void) {
    TEST_CASE("control_unit: j y jr activan Jump/JumpReg, no escriben registro");
    control_t j = control_unit(OP_J, 0);
    CHECK(j.Jump == 1);
    CHECK(j.RegWrite == 0);

    control_t jr = control_unit(OP_RTYPE, FN_JR);
    CHECK(jr.JumpReg == 1);
    CHECK(jr.RegWrite == 0);
    TEST_END();
}

static void test_decode_integration(void) {
    TEST_CASE("decode(): combina extract_fields + control + sign_extend + reg_read");
    int32_t regs[NUM_REGS] = {0};
    regs[1] = 50;
    uint32_t instr = ASM_ADDI(2, 1, 7); /* addi $2, $1, 7 */
    decode_out_t d = decode(instr, 4, regs);
    CHECK(d.val_rs == 50);
    CHECK(d.imm32 == 7);
    CHECK(d.control.ALUSrc == 1);
    CHECK(d.control.RegWrite == 1);
    TEST_END();
}

int main(void) {
    printf("== Unit tests: decode() ==\n");
    test_extract_fields_rtype();
    test_extract_fields_itype();
    test_extract_fields_jtype();
    test_sign_extend();
    test_register_read();
    test_control_unit_rtype();
    test_control_unit_lw_sw();
    test_control_unit_branch();
    test_control_unit_jump();
    test_decode_integration();
    TEST_SUMMARY("decode");
}
