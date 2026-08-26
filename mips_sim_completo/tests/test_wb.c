/* =====================================================================
 * test_wb.c -- Unit test de la ETAPA 5 (Write Back)
 *
 * Casos: los dos caminos del mux memToReg, instruccion que no escribe,
 *        guarda de $0 y burbuja.
 * ===================================================================== */
#include <string.h>
#include "minitest.h"
#include "mips.h"

static MEMWB memwb_base(uint8_t reg, uint32_t alu, uint32_t mem)
{
    MEMWB x;
    memset(&x, 0, sizeof(x));
    x.valid       = 1;
    x.c.reg_write = 1u;
    x.write_reg   = reg;
    x.alu_result  = alu;
    x.read_data   = mem;
    return x;
}

int main(void)
{
    RegFile rf;
    MEMWB   x;
    WBOut   out;

    MT_BEGIN("Etapa 5: Write Back");

    /* --- memToReg = 0: se escribe el resultado de la ALU --- */
    regfile_init(&rf);
    x = memwb_base(3u, 0xAAu, 0xBBu);
    out = stage_wb(&x, &rf);
    CHECK(out.wrote == 1, "la escritura se realizo");
    CHECK_EQ(rf.r[3], 0xAAu, "memToReg=0 escribe el resultado de la ALU");

    /* --- memToReg = 1: se escribe el dato leido de memoria --- */
    regfile_init(&rf);
    x = memwb_base(4u, 0xAAu, 0xBBu);
    x.c.mem_to_reg = 1u;
    out = stage_wb(&x, &rf);
    CHECK_EQ(rf.r[4], 0xBBu, "memToReg=1 escribe el dato de memoria");
    CHECK_EQ(out.value, 0xBBu, "el valor escrito se reporta");

    /* --- regWrite = 0 (sw, beq, j): no se toca el banco --- */
    regfile_init(&rf);
    rf.r[5] = 0x55u;
    x = memwb_base(5u, 0xAAu, 0xBBu);
    x.c.reg_write = 0u;
    out = stage_wb(&x, &rf);
    CHECK(out.wrote == 0, "sin regWrite no se escribe");
    CHECK_EQ(rf.r[5], 0x55u, "el registro conserva su valor");

    /* --- guarda de $0: nunca se escribe --- */
    regfile_init(&rf);
    x = memwb_base(0u, 0x99u, 0x99u);
    out = stage_wb(&x, &rf);
    CHECK(out.wrote == 0, "no se escribe $0");
    CHECK_EQ(rf.r[0], 0u, "$0 sigue valiendo cero");

    /* --- burbuja y parametros nulos --- */
    regfile_init(&rf);
    memset(&x, 0, sizeof(x));
    x.c.reg_write = 1u;
    x.write_reg = 6u;
    x.alu_result = 0x77u;
    out = stage_wb(&x, &rf);
    CHECK(out.wrote == 0, "una burbuja no escribe");
    CHECK_EQ(rf.r[6], 0u, "el banco no cambia con la burbuja");

    out = stage_wb(NULL, &rf);
    CHECK(out.wrote == 0, "entrada nula no escribe");

    return MT_END();
}
