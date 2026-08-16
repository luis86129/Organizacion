#include "mips_sim.h"
#include "mini_asm.h"
#include "test_common.h"

/*
 * Test de sistema: un programa MIPS ensamblado a mano que usa las 13
 * instrucciones al menos una vez, incluyendo un branch tomado (beq), uno
 * no tomado en la otra direccion (bne tomado en sentido distinto), un
 * jump incondicional (j) y un jump por registro (jr). Verifica el estado
 * final de registros y memoria, tal como se definio en el plan de
 * verificacion (vector test suite) del rol de Critico.
 *
 * Direcciones (indice de palabra -> direccion en bytes = indice * 4):
 *   0  addi $1, $0, 5
 *   1  addi $2, $0, 3
 *   2  add  $3, $1, $2        -> $3 = 8
 *   3  sub  $4, $1, $2        -> $4 = 2
 *   4  and  $5, $1, $2        -> $5 = 1
 *   5  or   $6, $1, $2        -> $6 = 7
 *   6  nor  $7, $1, $2        -> $7 = -8
 *   7  xor  $8, $1, $2        -> $8 = 6
 *   8  sw   $3, 0($0)         -> mem[0] = 8
 *   9  lw   $9, 0($0)         -> $9 = 8
 *   10 beq  $1, $1, 2         -> tomado, salta instrucciones 11 y 12
 *   11 addi $10, $0, 111      -> NO se ejecuta
 *   12 addi $10, $0, 222      -> NO se ejecuta
 *   13 bne  $1, $2, 1         -> tomado ($1 != $2), salta instruccion 14
 *   14 addi $11, $0, 333      -> NO se ejecuta
 *   15 addi $11, $0, 444      -> $11 = 444
 *   16 j    18                -> salta instruccion 17
 *   17 addi $12, $0, 555      -> NO se ejecuta
 *   18 addi $15, $0, 84       -> $15 = direccion de la instruccion 21
 *   19 jr   $15                -> salta a la instruccion 21
 *   20 addi $17, $0, 777      -> NO se ejecuta
 *   21 addi $18, $0, 888      -> $18 = 888
 *   22 HALT
 */
static void load_system_test_program(cpu_state_t *cpu) {
    int i = 0;
    cpu->instr_mem[i++] = ASM_ADDI(1, 0, 5);
    cpu->instr_mem[i++] = ASM_ADDI(2, 0, 3);
    cpu->instr_mem[i++] = ASM_ADD(3, 1, 2);
    cpu->instr_mem[i++] = ASM_SUB(4, 1, 2);
    cpu->instr_mem[i++] = ASM_AND(5, 1, 2);
    cpu->instr_mem[i++] = ASM_OR(6, 1, 2);
    cpu->instr_mem[i++] = ASM_NOR(7, 1, 2);
    cpu->instr_mem[i++] = ASM_XOR(8, 1, 2);
    cpu->instr_mem[i++] = ASM_SW(3, 0, 0);
    cpu->instr_mem[i++] = ASM_LW(9, 0, 0);
    cpu->instr_mem[i++] = ASM_BEQ(1, 1, 2);
    cpu->instr_mem[i++] = ASM_ADDI(10, 0, 111);
    cpu->instr_mem[i++] = ASM_ADDI(10, 0, 222);
    cpu->instr_mem[i++] = ASM_BNE(1, 2, 1);
    cpu->instr_mem[i++] = ASM_ADDI(11, 0, 333);
    cpu->instr_mem[i++] = ASM_ADDI(11, 0, 444);
    cpu->instr_mem[i++] = ASM_J(18);
    cpu->instr_mem[i++] = ASM_ADDI(12, 0, 555);
    cpu->instr_mem[i++] = ASM_ADDI(15, 0, 84);
    cpu->instr_mem[i++] = ASM_JR(15);
    cpu->instr_mem[i++] = ASM_ADDI(17, 0, 777);
    cpu->instr_mem[i++] = ASM_ADDI(18, 0, 888);
    cpu->instr_mem[i++] = ASM_HALT();
}

static void test_full_program(void) {
    TEST_CASE("Programa completo: aritmetica/logica, memoria, beq, bne, j, jr");

    cpu_state_t cpu;
    cpu_init(&cpu);
    load_system_test_program(&cpu);

    int guard = 0;
    while (!cpu.halted && guard < 100) {
        cpu_step(&cpu);
        guard++;
    }
    CHECK(cpu.halted == 1);

    /* Aritmetica y logica */
    CHECK(cpu.regs[1] == 5);
    CHECK(cpu.regs[2] == 3);
    CHECK(cpu.regs[3] == 8);    /* add */
    CHECK(cpu.regs[4] == 2);    /* sub */
    CHECK(cpu.regs[5] == 1);    /* and */
    CHECK(cpu.regs[6] == 7);    /* or  */
    CHECK(cpu.regs[7] == -8);   /* nor */
    CHECK(cpu.regs[8] == 6);    /* xor */

    /* Memoria */
    CHECK(cpu.data_mem[0] == 8);  /* sw   */
    CHECK(cpu.regs[9] == 8);      /* lw   */

    /* beq tomado: $10 nunca se escribe */
    CHECK(cpu.regs[10] == 0);

    /* bne tomado: $11 termina en 444, no en 333 */
    CHECK(cpu.regs[11] == 444);

    /* j: $12 nunca se escribe */
    CHECK(cpu.regs[12] == 0);

    /* jr: $17 nunca se escribe, $18 si */
    CHECK(cpu.regs[15] == 84);
    CHECK(cpu.regs[17] == 0);
    CHECK(cpu.regs[18] == 888);

    /* $0 nunca cambia, pase lo que pase */
    CHECK(cpu.regs[0] == 0);

    TEST_END();
}

/* Caso limite: $0 no debe modificarse aunque una instruccion intente
 * usarlo como destino. */
static void test_zero_register_immutable(void) {
    TEST_CASE("Caso limite: escribir en $0 (add $0,$1,$2) no lo modifica");
    cpu_state_t cpu;
    cpu_init(&cpu);
    cpu.instr_mem[0] = ASM_ADDI(1, 0, 99);
    cpu.instr_mem[1] = ASM_ADD(0, 1, 1); /* intenta escribir $0 = 198 */
    cpu.instr_mem[2] = ASM_HALT();

    int guard = 0;
    while (!cpu.halted && guard < 10) { cpu_step(&cpu); guard++; }

    CHECK(cpu.regs[0] == 0);
    TEST_END();
}

int main(void) {
    printf("== Test de sistema ==\n");
    test_full_program();
    test_zero_register_immutable();
    TEST_SUMMARY("sistema");
}
