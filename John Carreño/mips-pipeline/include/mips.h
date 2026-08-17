/* =====================================================================
 * mips.h -- Tipos, constantes y prototipos del simulador MIPS
 *           con pipeline de 5 etapas.
 *
 * Convencion de diseno (Proyecto Parte 1):
 *   - Las cinco etapas son funciones que reciben TODO su dato por
 *     parametro y devuelven una estructura de salida. No hay variables
 *     globales: el estado vive en InstrMem / DataMem / RegFile / Pipeline.
 *   - Los registros de pipeline (IFID, IDEX, EXMEM, MEMWB) son structs.
 *   - El ciclo se ejecuta en orden inverso (WB, MEM, EX, ID, IF) para que
 *     una instruccion no atraviese las cinco etapas en un solo ciclo.
 * ===================================================================== */
#ifndef MIPS_H
#define MIPS_H

#include <stdint.h>
#include <stddef.h>

/* --------------------------------------------------------------------
 * Capacidades. Las memorias se manejan siempre en palabras de 32 bits.
 * -------------------------------------------------------------------- */
#define IMEM_WORDS 1024u   /* 4 KB de memoria de instrucciones */
#define DMEM_WORDS 1024u   /* 4 KB de memoria de datos         */
#define NUM_REGS     32u

/* --------------------------------------------------------------------
 * ISA implementada (13 instrucciones)
 * -------------------------------------------------------------------- */
#define OP_RTYPE 0x00u
#define OP_J     0x02u
#define OP_BEQ   0x04u
#define OP_BNE   0x05u
#define OP_ADDI  0x08u
#define OP_LW    0x23u
#define OP_SW    0x2Bu

#define F_JR  0x08u
#define F_ADD 0x20u
#define F_SUB 0x22u
#define F_AND 0x24u
#define F_OR  0x25u
#define F_XOR 0x26u
#define F_NOR 0x27u

/* aluOp[1:0]: lo que la unidad de control le dice a la ALU */
#define ALUOP_ADD   0x0u  /* addi, lw, sw            */
#define ALUOP_SUB   0x1u  /* beq, bne                */
#define ALUOP_FUNCT 0x2u  /* tipo R: decide el funct */

/* aluControl[3:0]: operacion final de la ALU */
#define ALUC_AND 0x0u
#define ALUC_OR  0x1u
#define ALUC_ADD 0x2u
#define ALUC_XOR 0x3u
#define ALUC_SUB 0x6u
#define ALUC_NOR 0xCu

/* --------------------------------------------------------------------
 * Errores. El simulador nunca aborta: marca el error y detiene el run.
 * -------------------------------------------------------------------- */
typedef enum {
    MIPS_OK = 0,
    MIPS_ERR_ALIGN,    /* direccion no multiplo de 4            */
    MIPS_ERR_RANGE,    /* direccion fuera de la memoria         */
    MIPS_ERR_ILLEGAL,  /* opcode o funct no reconocido          */
    MIPS_ERR_NULL      /* parametro nulo (validacion de entrada)*/
} MipsError;

const char *mips_error_str(MipsError e);

/* --------------------------------------------------------------------
 * Estado: memorias y banco de registros
 * -------------------------------------------------------------------- */
typedef struct {
    uint32_t w[IMEM_WORDS];
    uint32_t n_words;      /* palabras realmente cargadas */
} InstrMem;

typedef struct {
    uint32_t w[DMEM_WORDS];
    uint32_t n_words;      /* palabra mas alta usada + 1  */
} DataMem;

typedef struct {
    uint32_t r[NUM_REGS];
} RegFile;

/* Carga/volcado de archivos .bin (palabras de 32 bits, little-endian). */
void mem_init_imem(InstrMem *im);
void mem_init_dmem(DataMem *dm);
void regfile_init(RegFile *rf);

int mem_load_imem(InstrMem *im, const char *path);
int mem_load_dmem(DataMem *dm, const char *path);
int regfile_load(RegFile *rf, const char *path);
int mem_dump_words(const uint32_t *w, uint32_t n_words, const char *path);

/* --------------------------------------------------------------------
 * Senales de control (salida de la unidad de control en ID)
 * -------------------------------------------------------------------- */
typedef struct {
    uint8_t reg_dst;    /* 1: destino rd (tipo R), 0: destino rt      */
    uint8_t alu_src;    /* 1: segundo operando = inmediato            */
    uint8_t alu_op;     /* ALUOP_*                                    */
    uint8_t mem_read;   /* lw                                         */
    uint8_t mem_write;  /* sw                                         */
    uint8_t mem_to_reg; /* 1: al banco va el dato leido de memoria    */
    uint8_t reg_write;  /* escribe en el banco de registros           */
    uint8_t branch;     /* beq o bne                                  */
    uint8_t branch_ne;  /* 1: bne, 0: beq                             */
    uint8_t jump;       /* j                                          */
    uint8_t jump_reg;   /* jr                                         */
} Control;

/* --------------------------------------------------------------------
 * Registros de pipeline. valid = 0 significa burbuja (NOP inyectada).
 * -------------------------------------------------------------------- */
typedef struct {
    int      valid;
    uint32_t instr;      /* instr[31:0]   */
    uint32_t pc_plus4;   /* PC+4          */
} IFID;

typedef struct {
    int      valid;
    Control  c;
    uint32_t rs_val;         /* valor de $rs                */
    uint32_t rt_val;         /* valor de $rt                */
    uint32_t imm32;          /* inmediato con signo extendido */
    uint32_t pc_plus4;
    uint32_t branch_target;  /* PC+4 + (imm << 2)           */
    uint32_t jump_target;    /* (PC+4)[31:28] | (addr << 2) */
    uint8_t  rs, rt, rd;     /* indices de 5 bits           */
    uint8_t  funct;
} IDEX;

typedef struct {
    int      valid;
    Control  c;
    uint32_t alu_result;
    uint32_t rt_val;      /* dato a escribir en memoria (sw) */
    uint8_t  write_reg;   /* destino ya resuelto por el mux regDst */
    int      zero;
    int      branch_taken;/* incluye beq/bne tomados, j y jr  */
    uint32_t branch_pc;   /* destino cuando branch_taken = 1  */
} EXMEM;

typedef struct {
    int      valid;
    Control  c;
    uint32_t alu_result;
    uint32_t read_data;
    uint8_t  write_reg;
} MEMWB;

/* =====================================================================
 * ETAPA 1: Instruction Fetch + Program Counter
 * ===================================================================== */
typedef struct {
    uint32_t pc;      /* PC actual: de aqui se lee la instruccion */
    int      taken;   /* 1: EX resolvio un salto tomado           */
    uint32_t target;  /* destino del salto                        */
} IFIn;

typedef struct {
    IFID      ifid;     /* lo que se carga en el registro IF/ID */
    uint32_t  next_pc;  /* PC del siguiente ciclo               */
    MipsError error;
} IFOut;

IFOut stage_if(const IFIn *in, const InstrMem *im);

/* =====================================================================
 * ETAPA 2: Instruction Decode
 * ===================================================================== */
typedef struct {
    IDEX      idex;
    MipsError error;
} IDOut;

/* Campos crudos de la instruccion (subfuncion splitFields). */
typedef struct {
    uint8_t  op, rs, rt, rd, shamt, funct;
    uint16_t imm16;
    uint32_t addr26;
} Fields;

Fields   id_split_fields(uint32_t instr);
int      id_control_unit(uint8_t op, uint8_t funct, Control *out); /* 0 ok, -1 ilegal */
uint32_t id_sign_extend(uint16_t imm16);

IDOut stage_id(const IFID *in, const RegFile *rf);

/* =====================================================================
 * ETAPA 3: ALU (Execute)
 * ===================================================================== */
typedef struct {
    EXMEM     exmem;
    MipsError error;
} EXOut;

int      ex_alu_control(uint8_t alu_op, uint8_t funct);          /* -1 ilegal */
uint32_t ex_execute(uint32_t a, uint32_t b, uint8_t alu_ctrl);

EXOut stage_ex(const IDEX *in);

/* =====================================================================
 * ETAPA 4: Memory
 * ===================================================================== */
typedef struct {
    MEMWB     memwb;
    MipsError error;
} MEMOut;

MEMOut stage_mem(const EXMEM *in, DataMem *dm);

/* =====================================================================
 * ETAPA 5: Write Back
 * ===================================================================== */
typedef struct {
    int      wrote;  /* 1 si realmente se escribio el banco */
    uint8_t  reg;
    uint32_t value;
} WBOut;

WBOut stage_wb(const MEMWB *in, RegFile *rf);

/* =====================================================================
 * Pipeline: orquesta las cinco etapas, los riesgos y los flush.
 * ===================================================================== */
typedef struct {
    uint32_t pc;
    IFID     ifid;
    IDEX     idex;
    EXMEM    exmem;
    MEMWB    memwb;

    uint64_t cycles;        /* ciclos ejecutados             */
    uint64_t instructions;  /* instrucciones completadas (WB)*/
    uint64_t stalls;        /* burbujas por riesgo de datos  */
    uint64_t flushes;       /* instrucciones descartadas     */

    MipsError error;
    int       halted;
} Pipeline;

void pipeline_init(Pipeline *p, uint32_t start_pc);
int  pipeline_step(Pipeline *p, const InstrMem *im, DataMem *dm, RegFile *rf);
int  pipeline_run(Pipeline *p, const InstrMem *im, DataMem *dm, RegFile *rf,
                  uint64_t max_cycles);
int  hazard_detect(const IFID *ifid, const IDEX *idex, const EXMEM *exmem);

#endif /* MIPS_H */
