#include "mips_sim.h"

/* pc_increment: PC -> PC+4
 * Cada instrucción ocupa 4 bytes (una palabra de 32 bits). */
uint32_t pc_increment(uint32_t pc) {
    return pc + 4;
}

/* instruction_read: PC, instr_mem[] -> instr[31:0]
 * La memoria de instrucciones está indexada por palabra, así que
 * convertimos la dirección en bytes (PC) a índice de palabra (PC/4). */
uint32_t instruction_read(const uint32_t *instr_mem, uint32_t pc) {
    return instr_mem[pc / 4];
}

/* pc_branch_update: branch_target, taken -> PC
 * Si la instrucción de decode resolvió un salto tomado (branch o jump),
 * el siguiente PC es el destino del salto; si no, es PC+4 (secuencial). */
uint32_t pc_branch_update(uint32_t pc_plus4, uint32_t branch_target, int taken) {
    return taken ? branch_target : pc_plus4;
}

/* fetch(): función principal de la etapa IF + PC.
 * Entradas: memoria de instrucciones, PC actual.
 * Salidas: instrucción leída y PC+4 (el PC final, considerando saltos,
 * se resuelve más adelante con pc_branch_update una vez que decode/alu
 * calculan el destino). */
fetch_out_t fetch(const uint32_t *instr_mem, uint32_t pc) {
    fetch_out_t out;
    out.instr = instruction_read(instr_mem, pc);
    out.pc_plus4 = pc_increment(pc);
    return out;
}
