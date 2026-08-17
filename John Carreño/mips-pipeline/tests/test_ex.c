/* =====================================================================
 * test_ex.c -- Unit test de la ETAPA 3 (ALU / Execute)
 *
 * Casos: aluControl, las seis operaciones, flag zero, mux de operandos,
 *        mux regDst, resolucion de beq/bne/j/jr y funct ilegal.
 * ===================================================================== */
#include <string.h>
#include "minitest.h"
#include "mips.h"

/* Arma un IDEX de tipo R listo para la ALU. */
static IDEX idex_tipo_r(uint32_t a, uint32_t b, uint8_t funct, uint8_t rd)
{
    IDEX x;
    memset(&x, 0, sizeof(x));
    x.valid       = 1;
    x.c.reg_dst   = 1u;
    x.c.alu_op    = ALUOP_FUNCT;
    x.c.reg_write = 1u;
    x.rs_val      = a;
    x.rt_val      = b;
    x.rd          = rd;
    x.funct       = funct;
    return x;
}

int main(void)
{
    IDEX  x;
    EXOut out;

    MT_BEGIN("Etapa 3: ALU (Execute)");

    /* --- aluControl --- */
    CHECK_EQ(ex_alu_control(ALUOP_ADD, 0), ALUC_ADD, "aluOp add");
    CHECK_EQ(ex_alu_control(ALUOP_SUB, 0), ALUC_SUB, "aluOp sub");
    CHECK_EQ(ex_alu_control(ALUOP_FUNCT, F_AND), ALUC_AND, "funct and");
    CHECK_EQ(ex_alu_control(ALUOP_FUNCT, F_OR),  ALUC_OR,  "funct or");
    CHECK_EQ(ex_alu_control(ALUOP_FUNCT, F_XOR), ALUC_XOR, "funct xor");
    CHECK_EQ(ex_alu_control(ALUOP_FUNCT, F_NOR), ALUC_NOR, "funct nor");
    CHECK_EQ(ex_alu_control(ALUOP_FUNCT, F_SUB), ALUC_SUB, "funct sub");
    CHECK(ex_alu_control(ALUOP_FUNCT, 0x3Fu) == -1, "funct ilegal");

    /* --- las seis operaciones --- */
    CHECK_EQ(ex_execute(10u, 3u, ALUC_ADD), 13u, "add");
    CHECK_EQ(ex_execute(10u, 3u, ALUC_SUB), 7u, "sub");
    CHECK_EQ(ex_execute(0x00FFu, 0x000Fu, ALUC_AND), 0x000Fu, "and");
    CHECK_EQ(ex_execute(0x00FFu, 0x000Fu, ALUC_OR),  0x00FFu, "or");
    CHECK_EQ(ex_execute(0x00FFu, 0x000Fu, ALUC_XOR), 0x00F0u, "xor");
    CHECK_EQ(ex_execute(0x00FFu, 0x000Fu, ALUC_NOR), 0xFFFFFF00u, "nor");
    CHECK_EQ(ex_execute(0xFFFFFFFFu, 1u, ALUC_ADD), 0u, "add con acarreo envuelve");
    CHECK_EQ(ex_execute(0x7FFFFFFFu, 1u, ALUC_ADD), 0x80000000u,
             "overflow con signo no lanza excepcion");
    CHECK_EQ(ex_execute(0u, 1u, ALUC_SUB), 0xFFFFFFFFu, "resta negativa");

    /* --- tipo R completo: resultado, destino y flag zero --- */
    x = idex_tipo_r(10u, 3u, F_ADD, 7u);
    out = stage_ex(&x);
    CHECK(out.error == MIPS_OK, "tipo R sin error");
    CHECK_EQ(out.exmem.alu_result, 13u, "resultado de add");
    CHECK_EQ(out.exmem.write_reg, 7u, "regDst elige rd");
    CHECK(out.exmem.zero == 0, "flag zero apagado");
    CHECK(out.exmem.branch_taken == 0, "tipo R no salta");

    x = idex_tipo_r(5u, 5u, F_SUB, 7u);
    out = stage_ex(&x);
    CHECK_EQ(out.exmem.alu_result, 0u, "5-5 = 0");
    CHECK(out.exmem.zero == 1, "flag zero encendido");

    /* --- mux aluSrc y regDst para tipo I (addi) --- */
    memset(&x, 0, sizeof(x));
    x.valid = 1; x.c.alu_src = 1u; x.c.alu_op = ALUOP_ADD;
    x.c.reg_write = 1u; x.c.reg_dst = 0u;
    x.rs_val = 100u; x.rt_val = 999u; x.imm32 = 5u; x.rt = 9u;
    out = stage_ex(&x);
    CHECK_EQ(out.exmem.alu_result, 105u, "aluSrc toma el inmediato");
    CHECK_EQ(out.exmem.write_reg, 9u, "regDst elige rt");

    /* --- beq: tomado cuando la resta da cero --- */
    memset(&x, 0, sizeof(x));
    x.valid = 1; x.c.branch = 1u; x.c.alu_op = ALUOP_SUB;
    x.rs_val = 8u; x.rt_val = 8u; x.branch_target = 0x200u;
    out = stage_ex(&x);
    CHECK(out.exmem.branch_taken == 1, "beq con operandos iguales se toma");
    CHECK_EQ(out.exmem.branch_pc, 0x200u, "destino de beq");

    x.rt_val = 9u;
    out = stage_ex(&x);
    CHECK(out.exmem.branch_taken == 0, "beq con operandos distintos no se toma");

    /* --- bne: exactamente al reves --- */
    x.c.branch_ne = 1u;
    out = stage_ex(&x);
    CHECK(out.exmem.branch_taken == 1, "bne con operandos distintos se toma");
    x.rt_val = 8u;
    out = stage_ex(&x);
    CHECK(out.exmem.branch_taken == 0, "bne con operandos iguales no se toma");

    /* --- j: siempre tomado, destino absoluto --- */
    memset(&x, 0, sizeof(x));
    x.valid = 1; x.c.jump = 1u; x.jump_target = 0x40u;
    out = stage_ex(&x);
    CHECK(out.exmem.branch_taken == 1, "j siempre se toma");
    CHECK_EQ(out.exmem.branch_pc, 0x40u, "destino de j");

    /* --- jr: toma el destino de $rs, no de $rd --- */
    memset(&x, 0, sizeof(x));
    x.valid = 1; x.c.jump_reg = 1u; x.rs_val = 0x80u; x.rd = 31u;
    out = stage_ex(&x);
    CHECK(out.exmem.branch_taken == 1, "jr siempre se toma");
    CHECK_EQ(out.exmem.branch_pc, 0x80u, "jr usa el valor de $rs");
    CHECK(out.exmem.c.reg_write == 0u, "jr no escribe registros");

    /* --- funct ilegal --- */
    x = idex_tipo_r(1u, 1u, 0x3Fu, 3u);
    out = stage_ex(&x);
    CHECK(out.error == MIPS_ERR_ILLEGAL, "funct ilegal reportado");

    /* --- burbuja y parametro nulo --- */
    memset(&x, 0, sizeof(x));
    out = stage_ex(&x);
    CHECK(out.exmem.valid == 0, "burbuja se propaga");
    out = stage_ex(NULL);
    CHECK(out.error == MIPS_ERR_NULL, "entrada nula se reporta");

    return MT_END();
}
