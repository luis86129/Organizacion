/* =====================================================================
 * test_mem.c -- Unit test de la ETAPA 4 (Memory)
 *
 * Casos: lw, sw, instruccion que no toca memoria, direccion desalineada,
 *        direccion fuera de rango y burbuja.
 * ===================================================================== */
#include <string.h>
#include "minitest.h"
#include "mips.h"

static EXMEM exmem_base(uint32_t addr, uint32_t rt_val)
{
    EXMEM x;
    memset(&x, 0, sizeof(x));
    x.valid      = 1;
    x.alu_result = addr;
    x.rt_val     = rt_val;
    x.write_reg  = 5u;
    return x;
}

int main(void)
{
    DataMem dm;
    EXMEM   x;
    MEMOut  out;

    MT_BEGIN("Etapa 4: Memory");

    mem_init_dmem(&dm);
    dm.w[4] = 0xDEADBEEFu;   /* direccion 16 */
    dm.n_words = 8u;

    /* --- lw: lee la palabra de la direccion 16 --- */
    x = exmem_base(16u, 0u);
    x.c.mem_read = 1u;
    x.c.mem_to_reg = 1u;
    x.c.reg_write = 1u;
    out = stage_mem(&x, &dm);
    CHECK(out.error == MIPS_OK, "lw alineado sin error");
    CHECK_EQ(out.memwb.read_data, 0xDEADBEEFu, "dato leido por lw");
    CHECK_EQ(out.memwb.write_reg, 5u, "destino propagado");
    CHECK(out.memwb.valid == 1, "lw produce MEM/WB valido");

    /* --- sw: escribe y no toca el banco de registros --- */
    x = exmem_base(20u, 0x12345678u);
    x.c.mem_write = 1u;
    out = stage_mem(&x, &dm);
    CHECK(out.error == MIPS_OK, "sw alineado sin error");
    CHECK_EQ(dm.w[5], 0x12345678u, "sw escribio la palabra");
    CHECK(out.memwb.c.reg_write == 0u, "sw no habilita escritura de registro");

    /* --- sw mas alla de lo cargado: n_words debe crecer --- */
    x = exmem_base(80u, 7u);
    x.c.mem_write = 1u;
    out = stage_mem(&x, &dm);
    CHECK_EQ(dm.w[20], 7u, "sw fuera del area cargada");
    CHECK_EQ(dm.n_words, 21u, "n_words crece con la escritura");

    /* --- instruccion que no toca memoria: solo deja pasar la ALU --- */
    x = exmem_base(0x99u, 0u);
    x.c.reg_write = 1u;
    out = stage_mem(&x, &dm);
    CHECK(out.error == MIPS_OK, "sin acceso a memoria no hay error");
    CHECK_EQ(out.memwb.alu_result, 0x99u, "resultado de ALU se deja pasar");
    CHECK_EQ(out.memwb.read_data, 0u, "sin lectura, read_data queda en cero");

    /* --- direccion desalineada --- */
    x = exmem_base(18u, 0u);
    x.c.mem_read = 1u;
    out = stage_mem(&x, &dm);
    CHECK(out.error == MIPS_ERR_ALIGN, "direccion 18 no esta alineada");

    /* --- direccion fuera de rango --- */
    x = exmem_base(DMEM_WORDS * 4u + 4u, 0u);
    x.c.mem_write = 1u;
    out = stage_mem(&x, &dm);
    CHECK(out.error == MIPS_ERR_RANGE, "direccion fuera de la memoria");

    /* --- burbuja y parametros nulos --- */
    memset(&x, 0, sizeof(x));
    out = stage_mem(&x, &dm);
    CHECK(out.memwb.valid == 0, "burbuja se propaga");
    out = stage_mem(NULL, &dm);
    CHECK(out.error == MIPS_ERR_NULL, "entrada nula se reporta");

    return MT_END();
}
