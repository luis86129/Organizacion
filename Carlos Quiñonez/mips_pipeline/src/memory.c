#include "mips_sim.h"
#include <stdio.h>
#include <stdlib.h>

/* Valida que una dirección de memoria (en bytes) esté dentro del rango
 * de la memoria de datos. Si no lo está, se aborta con un mensaje claro
 * en vez de dejar que el programa lea/escriba fuera de rango. */
static uint32_t validate_address(uint32_t address) {
    uint32_t index = address / 4;
    if (index >= DATA_MEM_WORDS) {
        fprintf(stderr,
                "Error: dirección de memoria fuera de rango (0x%08X)\n",
                address);
        exit(1);
    }
    return index;
}

/* mem_read: dirección(ALU) -> dato_leído[31:0] */
int32_t mem_read(const int32_t *data_mem, uint32_t address) {
    return data_mem[validate_address(address)];
}

/* mem_write: dirección, valor_rt -> data_mem[] actualizado */
void mem_write(int32_t *data_mem, uint32_t address, int32_t value) {
    data_mem[validate_address(address)] = value;
}

/* memory_stage(): función principal de la etapa Memory.
 * Solo actúa si la instrucción es lw (MemRead) o sw (MemWrite).
 * Para cualquier otra instrucción, el dato queda sin cambios (pasa
 * de largo hacia write_back). */
memory_out_t memory_stage(int32_t *data_mem, const control_t *control,
                           int32_t alu_result, int32_t val_rt) {
    memory_out_t out;
    out.mem_data = 0;

    if (control->MemRead) {
        out.mem_data = mem_read(data_mem, (uint32_t)alu_result);
    } else if (control->MemWrite) {
        mem_write(data_mem, (uint32_t)alu_result, val_rt);
        /* mem_data no aplica para sw; se deja en 0 (no se usa en write_back) */
    }
    return out;
}
