#include "mips_sim.h"
#include "test_common.h"
#include <string.h>

static void test_mem_write_read(void) {
    TEST_CASE("mem_write + mem_read: escribe y lee el mismo valor");
    int32_t data_mem[DATA_MEM_WORDS] = {0};
    mem_write(data_mem, 0, 123);
    mem_write(data_mem, 4, 456);
    CHECK(mem_read(data_mem, 0) == 123);
    CHECK(mem_read(data_mem, 4) == 456);
    TEST_END();
}

static void test_mem_boundary(void) {
    TEST_CASE("mem_read/mem_write: caso limite - ultima direccion valida");
    int32_t data_mem[DATA_MEM_WORDS] = {0};
    uint32_t last_addr = (DATA_MEM_WORDS - 1) * 4;
    mem_write(data_mem, last_addr, 999);
    CHECK(mem_read(data_mem, last_addr) == 999);
    TEST_END();
}

static void test_memory_stage_lw(void) {
    TEST_CASE("memory_stage(): lw lee de memoria (MemRead=1)");
    int32_t data_mem[DATA_MEM_WORDS] = {0};
    data_mem[2] = 77; /* direccion 8 */
    control_t c = {0};
    c.MemRead = 1;
    memory_out_t m = memory_stage(data_mem, &c, 8 /* alu_result = direccion */, 0);
    CHECK(m.mem_data == 77);
    TEST_END();
}

static void test_memory_stage_sw(void) {
    TEST_CASE("memory_stage(): sw escribe en memoria (MemWrite=1)");
    int32_t data_mem[DATA_MEM_WORDS] = {0};
    control_t c = {0};
    c.MemWrite = 1;
    memory_stage(data_mem, &c, 12 /* alu_result = direccion */, 555 /* val_rt */);
    CHECK(data_mem[3] == 555);
    TEST_END();
}

static void test_memory_stage_passthrough(void) {
    TEST_CASE("memory_stage(): instrucciones sin acceso a memoria no cambian nada");
    int32_t data_mem[DATA_MEM_WORDS] = {0};
    data_mem[0] = 42;
    control_t c = {0}; /* add, and, etc: MemRead=0, MemWrite=0 */
    memory_stage(data_mem, &c, 999, 999);
    CHECK(data_mem[0] == 42); /* memoria no se tocó */
    TEST_END();
}

int main(void) {
    printf("== Unit tests: memory() ==\n");
    test_mem_write_read();
    test_mem_boundary();
    test_memory_stage_lw();
    test_memory_stage_sw();
    test_memory_stage_passthrough();
    TEST_SUMMARY("memory");
}
