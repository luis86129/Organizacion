#include "mips_sim.h"
#include "test_common.h"

static void test_select_writeback_source(void) {
    TEST_CASE("select_writeback_source: elige ALU o memoria segun MemToReg");
    CHECK(select_writeback_source(10, 20, 0) == 10); /* viene de la ALU */
    CHECK(select_writeback_source(10, 20, 1) == 20); /* viene de memoria (lw) */
    TEST_END();
}

static void test_register_write_basic(void) {
    TEST_CASE("register_write: escribe cuando RegWrite=1");
    int32_t regs[NUM_REGS] = {0};
    register_write(regs, 5, 999, 1);
    CHECK(regs[5] == 999);
    TEST_END();
}

static void test_register_write_disabled(void) {
    TEST_CASE("register_write: no escribe cuando RegWrite=0 (ej. sw, beq)");
    int32_t regs[NUM_REGS] = {0};
    regs[5] = 111;
    register_write(regs, 5, 999, 0);
    CHECK(regs[5] == 111); /* no cambia */
    TEST_END();
}

static void test_register_write_protects_zero(void) {
    TEST_CASE("register_write: caso limite - $0 nunca se escribe");
    int32_t regs[NUM_REGS] = {0};
    register_write(regs, 0, 999, 1);
    CHECK(regs[0] == 0);
    TEST_END();
}

static void test_write_back_integration_rtype(void) {
    TEST_CASE("write_back(): instruccion tipo R escribe en rd, con dato de ALU");
    int32_t regs[NUM_REGS] = {0};
    decode_out_t d = {0};
    d.fields.rd = 3;
    d.fields.rt = 7;
    d.control.RegDst   = 1; /* destino = rd */
    d.control.RegWrite = 1;
    d.control.MemToReg = 0; /* dato viene de la ALU */
    write_back(regs, &d, 42 /* alu_result */, 0 /* mem_data */);
    CHECK(regs[3] == 42);
    TEST_END();
}

static void test_write_back_integration_lw(void) {
    TEST_CASE("write_back(): lw escribe en rt, con dato de memoria");
    int32_t regs[NUM_REGS] = {0};
    decode_out_t d = {0};
    d.fields.rd = 3;
    d.fields.rt = 7;
    d.control.RegDst   = 0; /* destino = rt (tipo I) */
    d.control.RegWrite = 1;
    d.control.MemToReg = 1; /* dato viene de memoria */
    write_back(regs, &d, 42 /* alu_result: no se usa */, 88 /* mem_data */);
    CHECK(regs[7] == 88);
    TEST_END();
}

int main(void) {
    printf("== Unit tests: write_back() ==\n");
    test_select_writeback_source();
    test_register_write_basic();
    test_register_write_disabled();
    test_register_write_protects_zero();
    test_write_back_integration_rtype();
    test_write_back_integration_lw();
    TEST_SUMMARY("write_back");
}
