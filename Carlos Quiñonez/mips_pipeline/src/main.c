#include <stdio.h>
#include "mips_sim.h"
#include "mini_asm.h"

/* Programa de demostración:
 *   addi $t0, $0, 10      # $t0 = 10
 *   addi $t1, $0, 20      # $t1 = 20
 *   add  $t2, $t0, $t1    # $t2 = 30
 *   sw   $t2, 0($0)       # mem[0] = 30
 *   lw   $t3, 0($0)       # $t3 = mem[0] = 30
 *   sub  $t4, $t3, $t0    # $t4 = 20
 *   halt
 */
static void load_demo_program(cpu_state_t *cpu) {
    int i = 0;
    cpu->instr_mem[i++] = ASM_ADDI(8, 0, 10);   /* $t0 = $8  */
    cpu->instr_mem[i++] = ASM_ADDI(9, 0, 20);   /* $t1 = $9  */
    cpu->instr_mem[i++] = ASM_ADD(10, 8, 9);     /* $t2 = $10 */
    cpu->instr_mem[i++] = ASM_SW(10, 0, 0);
    cpu->instr_mem[i++] = ASM_LW(11, 0, 0);      /* $t3 = $11 */
    cpu->instr_mem[i++] = ASM_SUB(12, 11, 8);    /* $t4 = $12 */
    cpu->instr_mem[i++] = ASM_HALT();
}

static void print_state(const cpu_state_t *cpu) {
    printf("PC final: 0x%08X\n", cpu->pc);
    for (int r = 8; r <= 12; r++) {
        printf("  $%-2d = %d\n", r, cpu->regs[r]);
    }
    printf("  mem[0] = %d\n", cpu->data_mem[0]);
}

int main(void) {
    cpu_state_t cpu;
    cpu_init(&cpu);
    load_demo_program(&cpu);

    while (!cpu.halted) {
        cpu_step(&cpu);
    }

    printf("Programa de demostración ejecutado.\n\n");
    print_state(&cpu);
    return 0;
}
