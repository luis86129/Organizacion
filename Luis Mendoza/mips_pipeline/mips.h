#ifndef MIPS_H
#define MIPS_H

#include <stdint.h>
#include <stddef.h>

#define NUM_REGS 32
#define MAX_MEMORY_WORDS 1024
#define MAX_PROGRAM 1024

/* Banco de registros */
typedef struct {
    uint32_t regs[NUM_REGS];
} RegisterFile;

/* Memoria */
typedef struct {
    uint32_t address;
    uint32_t value;
} MemoryWord;

typedef struct {
    MemoryWord words[MAX_MEMORY_WORDS];
    size_t count;
} Memory;

/* Instrucción decodificada */
typedef struct {
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint8_t shamt;
    uint8_t funct;

    int32_t immediate;
    uint32_t address;

    uint32_t readData1;
    uint32_t readData2;

} DecodedInstruction;

/* Procesador */
typedef struct {

    RegisterFile rf;
    Memory memory;

    uint32_t pc;

    uint32_t program[MAX_PROGRAM];
    size_t programSize;

} MIPS;


/* =========================================================
   1. INSTRUCTION FETCH + PROGRAM COUNTER
   ========================================================= */

uint32_t instruction_fetch(
    const MIPS *cpu,
    uint32_t pc,
    uint32_t *next_pc
);


/* =========================================================
   2. INSTRUCTION DECODE
   ========================================================= */

int instruction_decode(
    uint32_t instruction,
    const RegisterFile *rf,
    DecodedInstruction *out
);


/* =========================================================
   3. ALU
   ========================================================= */

uint32_t alu_execute(
    uint32_t a,
    uint32_t b,
    uint8_t operation,
    uint8_t *zero
);


/* =========================================================
   4. MEMORY
   ========================================================= */

int memory_read(
    const Memory *memory,
    uint32_t address,
    uint32_t *value
);

int memory_write(
    Memory *memory,
    uint32_t address,
    uint32_t value
);


/* =========================================================
   5. WRITE BACK
   ========================================================= */

void write_back(
    RegisterFile *rf,
    uint8_t reg,
    uint32_t value,
    uint8_t reg_write
);


/* =========================================================
   ARCHIVOS
   ========================================================= */

int load_binary_lines(
    const char *filename,
    uint32_t *buffer,
    size_t max,
    size_t *count
);

int load_registers(
    const char *filename,
    RegisterFile *rf
);

int load_memory(
    const char *filename,
    Memory *memory
);


/* =========================================================
   SIMULADOR
   ========================================================= */

int execute_program(
    MIPS *cpu,
    size_t max_cycles
);

void print_registers(
    const RegisterFile *rf
);


/* =========================================================
   FUNCIONES PARA CREAR INSTRUCCIONES EN LOS TESTS
   ========================================================= */

uint32_t enc_r(
    uint8_t rs,
    uint8_t rt,
    uint8_t rd,
    uint8_t funct
);

uint32_t enc_i(
    uint8_t opcode,
    uint8_t rs,
    uint8_t rt,
    int16_t immediate
);

uint32_t enc_j(
    uint8_t opcode,
    uint32_t address
);

#endif