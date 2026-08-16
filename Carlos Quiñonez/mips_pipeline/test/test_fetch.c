#include "mips_sim.h"
#include "test_common.h"

static void test_pc_increment(void) {
    TEST_CASE("pc_increment: PC avanza en 4");
    CHECK(pc_increment(0) == 4);
    CHECK(pc_increment(100) == 104);
    TEST_END();
}

static void test_instruction_read(void) {
    TEST_CASE("instruction_read: lee la palabra correcta segun PC");
    uint32_t mem[4] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    CHECK(instruction_read(mem, 0) == 0x11111111);
    CHECK(instruction_read(mem, 4) == 0x22222222);
    CHECK(instruction_read(mem, 8) == 0x33333333);
    TEST_END();
}

static void test_pc_branch_update(void) {
    TEST_CASE("pc_branch_update: elige target si taken, si no PC+4");
    CHECK(pc_branch_update(104, 200, 1) == 200);  /* taken     */
    CHECK(pc_branch_update(104, 200, 0) == 104);  /* no taken  */
    TEST_END();
}

static void test_fetch_integration(void) {
    TEST_CASE("fetch(): combina lectura de instruccion y PC+4");
    uint32_t mem[2] = {0xDEADBEEF, 0xCAFEF00D};
    fetch_out_t f = fetch(mem, 0);
    CHECK(f.instr == 0xDEADBEEF);
    CHECK(f.pc_plus4 == 4);
    TEST_END();
}

/* Caso limite: PC apuntando a la ultima palabra valida del arreglo */
static void test_fetch_boundary(void) {
    TEST_CASE("fetch(): caso limite en el ultimo indice valido");
    uint32_t mem[INSTR_MEM_WORDS];
    mem[INSTR_MEM_WORDS - 1] = 0xABCD1234;
    uint32_t last_pc = (INSTR_MEM_WORDS - 1) * 4;
    CHECK(instruction_read(mem, last_pc) == 0xABCD1234);
    TEST_END();
}

int main(void) {
    printf("== Unit tests: fetch() ==\n");
    test_pc_increment();
    test_instruction_read();
    test_pc_branch_update();
    test_fetch_integration();
    test_fetch_boundary();
    TEST_SUMMARY("fetch");
}
