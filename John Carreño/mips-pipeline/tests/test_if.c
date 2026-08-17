/* =====================================================================
 * test_if.c -- Unit test de la ETAPA 1 (Instruction Fetch + PC)
 *
 * Casos: secuencia normal (PC+4), salto tomado, fin de programa,
 *        PC desalineado y parametros nulos.
 * ===================================================================== */
#include "minitest.h"
#include "mips.h"
#include "encode.h"

int main(void)
{
    InstrMem im;
    IFIn     in;
    IFOut    out;

    MT_BEGIN("Etapa 1: Instruction Fetch + Program Counter");

    mem_init_imem(&im);
    im.w[0] = ASM_ADDI(1, 0, 10);
    im.w[1] = ASM_ADDI(2, 0, 3);
    im.w[2] = ASM_ADD(3, 1, 2);
    im.n_words = 3u;

    /* --- caso 1: busqueda normal desde PC = 0 --- */
    in.pc = 0u; in.taken = 0; in.target = 0u;
    out = stage_if(&in, &im);
    CHECK(out.error == MIPS_OK, "PC=0 no debe dar error");
    CHECK(out.ifid.valid == 1, "PC=0 debe entregar instruccion valida");
    CHECK_EQ(out.ifid.instr, im.w[0], "instruccion buscada en PC=0");
    CHECK_EQ(out.ifid.pc_plus4, 4u, "pcPlus4 en PC=0");
    CHECK_EQ(out.next_pc, 4u, "nextPC secuencial");

    /* --- caso 2: la segunda palabra --- */
    in.pc = 4u;
    out = stage_if(&in, &im);
    CHECK_EQ(out.ifid.instr, im.w[1], "instruccion buscada en PC=4");
    CHECK_EQ(out.next_pc, 8u, "nextPC secuencial desde 4");

    /* --- caso 3: salto tomado, selectNextPC toma el destino --- */
    in.pc = 4u; in.taken = 1; in.target = 0x40u;
    out = stage_if(&in, &im);
    CHECK_EQ(out.next_pc, 0x40u, "nextPC con salto tomado");
    CHECK_EQ(out.ifid.pc_plus4, 8u, "pcPlus4 no cambia por el salto");

    /* --- caso 4: PC mas alla del programa: no hay instruccion, no hay error */
    in.pc = 12u; in.taken = 0; in.target = 0u;
    out = stage_if(&in, &im);
    CHECK(out.error == MIPS_OK, "pasar el fin del programa no es error");
    CHECK(out.ifid.valid == 0, "fin del programa entrega burbuja");

    /* --- caso 5: PC desalineado --- */
    in.pc = 6u;
    out = stage_if(&in, &im);
    CHECK(out.error == MIPS_ERR_ALIGN, "PC=6 debe reportar desalineacion");
    CHECK(out.ifid.valid == 0, "PC desalineado no entrega instruccion");
    CHECK_EQ(out.next_pc, 6u, "PC desalineado congela el avance");

    /* --- caso 6: parametros nulos --- */
    out = stage_if(NULL, &im);
    CHECK(out.error == MIPS_ERR_NULL, "entrada nula se reporta como error");
    out = stage_if(&in, NULL);
    CHECK(out.error == MIPS_ERR_NULL, "memoria nula se reporta como error");

    return MT_END();
}
