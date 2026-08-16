#include "mips_sim.h"
#include "test_common.h"

static void test_alu_execute_ops(void) {
    TEST_CASE("alu_execute: add, sub, and, or, nor, xor");
    CHECK(alu_execute(5, 3, ALU_ADD) == 8);
    CHECK(alu_execute(5, 3, ALU_SUB) == 2);
    CHECK(alu_execute(0xF0, 0x0F, ALU_AND) == 0x00);
    CHECK(alu_execute(0xF0, 0x0F, ALU_OR)  == 0xFF);
    CHECK(alu_execute(0xF0, 0x0F, ALU_NOR) == ~0xFF);
    CHECK(alu_execute(0xFF, 0x0F, ALU_XOR) == 0xF0);
    TEST_END();
}

static void test_alu_execute_overflow(void) {
    TEST_CASE("alu_execute: caso limite - overflow con enteros con signo");
    int32_t max = 2147483647;      /* INT32_MAX */
    int32_t result = alu_execute(max, 1, ALU_ADD);
    CHECK(result == -2147483648);  /* overflow: da la vuelta (comportamiento definido para wraparound) */
    TEST_END();
}

static void test_zero_flag(void) {
    TEST_CASE("zero_flag: detecta resultado igual a cero");
    CHECK(zero_flag(0) == 1);
    CHECK(zero_flag(5) == 0);
    CHECK(zero_flag(-5) == 0);
    TEST_END();
}

static void test_alu_integration_regtype(void) {
    TEST_CASE("alu(): usa val_rt cuando ALUSrc = 0 (tipo R)");
    decode_out_t d = {0};
    d.val_rs = 10;
    d.val_rt = 4;
    d.control.ALUSrc = 0;
    d.control.ALUOp  = ALU_SUB;
    alu_out_t a = alu(&d);
    CHECK(a.result == 6);
    CHECK(a.zero == 0);
    TEST_END();
}

static void test_alu_integration_itype(void) {
    TEST_CASE("alu(): usa imm32 cuando ALUSrc = 1 (tipo I, ej. addi/lw/sw)");
    decode_out_t d = {0};
    d.val_rs = 10;
    d.imm32  = 10;
    d.control.ALUSrc = 1;
    d.control.ALUOp  = ALU_SUB;
    alu_out_t a = alu(&d);
    CHECK(a.result == 0);
    CHECK(a.zero == 1);   /* util para beq: rs - rt == 0 -> iguales */
    TEST_END();
}

int main(void) {
    printf("== Unit tests: alu() ==\n");
    test_alu_execute_ops();
    test_alu_execute_overflow();
    test_zero_flag();
    test_alu_integration_regtype();
    test_alu_integration_itype();
    TEST_SUMMARY("alu");
}
