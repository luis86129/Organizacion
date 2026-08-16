#include "mips_sim.h"

/* select_writeback_source: resultado_ALU, dato_mem, MemToReg -> dato_final
 * Mux que decide si lo que se escribe en el banco de registros viene
 * de la ALU (la mayoría de instrucciones) o de memoria (lw). */
int32_t select_writeback_source(int32_t alu_result, int32_t mem_data, int mem_to_reg) {
    return mem_to_reg ? mem_data : alu_result;
}

/* register_write: dato_final, rd/rt, RegWrite -> banco_registros actualizado
 * Protege $0: en MIPS, el registro 0 siempre vale 0 y nunca se escribe. */
void register_write(int32_t *regs, uint32_t dest_reg, int32_t value, int reg_write) {
    if (reg_write && dest_reg != 0) {
        regs[dest_reg] = value;
    }
}

/* write_back(): función principal de la etapa Write Back.
 * Determina el registro destino según RegDst (rd para tipo R, rt para
 * tipo I) y escribe el dato final seleccionado por el mux. */
void write_back(int32_t *regs, const decode_out_t *d, int32_t alu_result,
                 int32_t mem_data) {
    uint32_t dest_reg = d->control.RegDst ? d->fields.rd : d->fields.rt;
    int32_t  final_val = select_writeback_source(alu_result, mem_data,
                                                  d->control.MemToReg);
    register_write(regs, dest_reg, final_val, d->control.RegWrite);
}
