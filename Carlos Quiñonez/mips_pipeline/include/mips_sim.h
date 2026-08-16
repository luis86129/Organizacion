#ifndef MIPS_SIM_H
#define MIPS_SIM_H

#include <stdint.h>

/* ===================== Constantes generales ===================== */

#define NUM_REGS        32      /* banco de registros: $0 .. $31            */
#define INSTR_MEM_WORDS 256      /* memoria de instrucciones (en palabras)   */
#define DATA_MEM_WORDS  256      /* memoria de datos (en palabras de 32b)    */

/* Opcodes (campo instr[31:26]) */
#define OP_RTYPE 0x00
#define OP_ADDI  0x08
#define OP_LW    0x23
#define OP_SW    0x2B
#define OP_BEQ   0x04
#define OP_BNE   0x05
#define OP_J     0x02

/* Funct (campo instr[5:0], solo para tipo R) */
#define FN_ADD 0x20
#define FN_SUB 0x22
#define FN_AND 0x24
#define FN_OR  0x25
#define FN_NOR 0x27
#define FN_XOR 0x26
#define FN_JR  0x08

/* Operaciones que puede ejecutar la ALU */
typedef enum {
    ALU_ADD,
    ALU_SUB,
    ALU_AND,
    ALU_OR,
    ALU_NOR,
    ALU_XOR,
    ALU_NOP   /* para instrucciones que no usan la ALU (j, jr) */
} alu_op_t;

/* ===================== Estado global del CPU ===================== */

typedef struct {
    int32_t  regs[NUM_REGS];               /* banco de registros        */
    uint32_t instr_mem[INSTR_MEM_WORDS];    /* memoria de instrucciones  */
    int32_t  data_mem[DATA_MEM_WORDS];      /* memoria de datos          */
    uint32_t pc;                            /* program counter           */
    int      halted;                        /* 1 si el programa terminó  */
} cpu_state_t;

void cpu_init(cpu_state_t *cpu);

/* ===================== Etapa 1: IF + Program Counter ===================== */

uint32_t pc_increment(uint32_t pc);
uint32_t instruction_read(const uint32_t *instr_mem, uint32_t pc);
uint32_t pc_branch_update(uint32_t pc_plus4, uint32_t branch_target, int taken);

typedef struct {
    uint32_t instr;     /* instrucción leída       */
    uint32_t pc_plus4;  /* PC secuencial siguiente */
} fetch_out_t;

fetch_out_t fetch(const uint32_t *instr_mem, uint32_t pc);

/* ===================== Etapa 2: Instruction Decode ===================== */

typedef struct {
    uint32_t opcode;
    uint32_t rs;
    uint32_t rt;
    uint32_t rd;
    uint32_t shamt;
    uint32_t funct;
    uint16_t imm16;
    uint32_t addr26;
} fields_t;

typedef struct {
    int RegDst;      /* 1 = destino es rd (tipo R), 0 = destino es rt (tipo I) */
    int ALUSrc;       /* 1 = segundo operando es el inmediato con signo extendido */
    int MemToReg;     /* 1 = dato a escribir viene de memoria, 0 = viene de la ALU */
    int RegWrite;     /* 1 = se escribe en el banco de registros */
    int MemRead;      /* 1 = lectura de memoria (lw) */
    int MemWrite;     /* 1 = escritura en memoria (sw) */
    int Branch;       /* 1 = instrucción es beq */
    int BranchNE;      /* 1 = instrucción es bne */
    int Jump;          /* 1 = instrucción es j */
    int JumpReg;        /* 1 = instrucción es jr */
    alu_op_t ALUOp;    /* operación que debe ejecutar la ALU */
} control_t;

typedef struct {
    fields_t fields;
    control_t control;
    int32_t  val_rs;
    int32_t  val_rt;
    int32_t  imm32;
    uint32_t pc_plus4;
} decode_out_t;

fields_t   extract_fields(uint32_t instr);
control_t  control_unit(uint32_t opcode, uint32_t funct);
int32_t    sign_extend(uint16_t imm16);
void       register_read(const int32_t *regs, uint32_t rs, uint32_t rt,
                          int32_t *val_rs, int32_t *val_rt);

decode_out_t decode(uint32_t instr, uint32_t pc_plus4, const int32_t *regs);

/* ===================== Etapa 3: ALU (Execute) ===================== */

typedef struct {
    int32_t result;
    int     zero;
} alu_out_t;

int32_t alu_execute(int32_t a, int32_t b, alu_op_t op);
int     zero_flag(int32_t result);

alu_out_t alu(const decode_out_t *d);

/* ===================== Etapa 4: Memory ===================== */

int32_t mem_read(const int32_t *data_mem, uint32_t address);
void    mem_write(int32_t *data_mem, uint32_t address, int32_t value);

typedef struct {
    int32_t mem_data;   /* dato leído de memoria (o sin cambios si no aplica) */
} memory_out_t;

memory_out_t memory_stage(int32_t *data_mem, const control_t *control,
                           int32_t alu_result, int32_t val_rt);

/* ===================== Etapa 5: Write Back ===================== */

int32_t select_writeback_source(int32_t alu_result, int32_t mem_data, int mem_to_reg);
void    register_write(int32_t *regs, uint32_t dest_reg, int32_t value, int reg_write);

void write_back(int32_t *regs, const decode_out_t *d, int32_t alu_result,
                 int32_t mem_data);

/* ===================== Ciclo completo (para el test de sistema) ===================== */

/* Ejecuta una instrucción completa (las 5 etapas) y actualiza cpu->pc.
 * Retorna 0 si se debe seguir ejecutando, 1 si se detectó fin de programa
 * (instrucción 0x00000000 usada como centinela HALT). */
int cpu_step(cpu_state_t *cpu);

#endif /* MIPS_SIM_H */
