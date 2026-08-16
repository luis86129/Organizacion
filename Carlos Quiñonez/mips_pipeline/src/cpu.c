#include "mips_sim.h"
#include <string.h>

void cpu_init(cpu_state_t *cpu) {
    memset(cpu, 0, sizeof(*cpu));
}

/* cpu_step(): ejecuta UNA instrucción completa a través de las 5 etapas.
 *
 * El simulador no modela solapamiento entre instrucciones (no hay
 * hazards de datos ni de control que resolver): cada instrucción
 * atraviesa fetch -> decode -> alu -> memory -> write_back por completo
 * antes de que el PC avance a la siguiente.
 *
 * Se usa la palabra 0x00000000 como centinela de fin de programa (HALT). */
int cpu_step(cpu_state_t *cpu) {
    /* --- Etapa 1: IF + PC --- */
    fetch_out_t f = fetch(cpu->instr_mem, cpu->pc);

    if (f.instr == 0x00000000) {
        cpu->halted = 1;
        return 1;
    }

    /* --- Etapa 2: Decode --- */
    decode_out_t d = decode(f.instr, f.pc_plus4, cpu->regs);

    /* --- Etapa 3: ALU --- */
    alu_out_t a = alu(&d);

    /* --- Resolución del próximo PC (branch / jump / jr / secuencial) --- */
    uint32_t branch_target = f.pc_plus4 + ((uint32_t)d.imm32 << 2);
    uint32_t jump_target   = (f.pc_plus4 & 0xF0000000u) | (d.fields.addr26 << 2);

    int branch_taken = (d.control.Branch   && a.zero) ||
                        (d.control.BranchNE && !a.zero);

    uint32_t target = branch_target;
    int taken = branch_taken;

    if (d.control.Jump) {
        target = jump_target;
        taken = 1;
    } else if (d.control.JumpReg) {
        target = (uint32_t)d.val_rs;
        taken = 1;
    }

    /* --- Etapa 4: Memory --- */
    memory_out_t m = memory_stage(cpu->data_mem, &d.control, a.result, d.val_rt);

    /* --- Etapa 5: Write Back --- */
    write_back(cpu->regs, &d, a.result, m.mem_data);

    /* --- Actualización final del PC (equivalente a pc_branch_update) --- */
    cpu->pc = pc_branch_update(f.pc_plus4, target, taken);

    return 0;
}
