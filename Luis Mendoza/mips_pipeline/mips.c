#include "mips.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* =========================================================
   OPCODES
   ========================================================= */

#define OP_RTYPE 0x00

#define OP_ADDI  0x08
#define OP_BEQ   0x04
#define OP_BNE   0x05
#define OP_LW    0x23
#define OP_SW    0x2B

#define OP_J     0x02


/* =========================================================
   FUNCT
   ========================================================= */

#define F_ADD 0x20
#define F_SUB 0x22
#define F_AND 0x24
#define F_OR  0x25
#define F_NOR 0x27
#define F_XOR 0x26
#define F_JR  0x08


/* =========================================================
   OPERACIONES DE LA ALU
   ========================================================= */

#define ALU_ADD 0
#define ALU_SUB 1
#define ALU_AND 2
#define ALU_OR  3
#define ALU_NOR 4
#define ALU_XOR 5


/* =========================================================
   1. INSTRUCTION FETCH + PROGRAM COUNTER
   ========================================================= */

uint32_t instruction_fetch(
    const MIPS *cpu,
    uint32_t pc,
    uint32_t *next_pc
) {

    /*
     * Cada instrucción ocupa 4 bytes.
     *
     * Por ejemplo:
     *
     * PC = 0
     * instrucción 0
     *
     * PC = 4
     * instrucción 1
     *
     * PC = 8
     * instrucción 2
     */

    uint32_t index = pc / 4;


    /* Siguiente instrucción */

    *next_pc = pc + 4;


    /* Si el PC está fuera del programa */

    if (index >= cpu->programSize) {

        return 0;
    }


    /* Retornar instrucción */

    return cpu->program[index];
}


/* =========================================================
   2. INSTRUCTION DECODE
   ========================================================= */

int instruction_decode(
    uint32_t instruction,
    const RegisterFile *rf,
    DecodedInstruction *out
) {

    memset(out, 0, sizeof(*out));


    /*
     * Formato MIPS:
     *
     * opcode = bits 31-26
     */

    out->opcode =
        (instruction >> 26) & 0x3F;


    /*
     * rs = bits 25-21
     */

    out->rs =
        (instruction >> 21) & 0x1F;


    /*
     * rt = bits 20-16
     */

    out->rt =
        (instruction >> 16) & 0x1F;


    /*
     * rd = bits 15-11
     */

    out->rd =
        (instruction >> 11) & 0x1F;


    /*
     * shamt = bits 10-6
     */

    out->shamt =
        (instruction >> 6) & 0x1F;


    /*
     * funct = bits 5-0
     */

    out->funct =
        instruction & 0x3F;


    /*
     * Immediate.
     *
     * Se convierte a int16_t para hacer
     * correctamente la extensión de signo.
     */

    out->immediate =
        (int16_t)(instruction & 0xFFFF);


    /*
     * Dirección para instrucciones J.
     */

    out->address =
        instruction & 0x03FFFFFF;


    /*
     * Leer registros.
     */

    out->readData1 =
        rf->regs[out->rs];

    out->readData2 =
        rf->regs[out->rt];


    /*
     * Verificar opcode válido.
     */

    if (
        out->opcode != OP_RTYPE &&
        out->opcode != OP_ADDI &&
        out->opcode != OP_BEQ &&
        out->opcode != OP_BNE &&
        out->opcode != OP_LW &&
        out->opcode != OP_SW &&
        out->opcode != OP_J
    ) {

        return -1;
    }


    /*
     * Si es R-Type verificar funct.
     */

    if (out->opcode == OP_RTYPE) {

        if (
            out->funct != F_ADD &&
            out->funct != F_SUB &&
            out->funct != F_AND &&
            out->funct != F_OR &&
            out->funct != F_NOR &&
            out->funct != F_XOR &&
            out->funct != F_JR
        ) {

            return -1;
        }
    }


    return 0;
}


/* =========================================================
   3. ALU
   ========================================================= */

uint32_t alu_execute(
    uint32_t a,
    uint32_t b,
    uint8_t operation,
    uint8_t *zero
) {

    uint32_t result = 0;


    switch (operation) {

        case ALU_ADD:

            result = a + b;

            break;


        case ALU_SUB:

            result = a - b;

            break;


        case ALU_AND:

            result = a & b;

            break;


        case ALU_OR:

            result = a | b;

            break;


        case ALU_NOR:

            result = ~(a | b);

            break;


        case ALU_XOR:

            result = a ^ b;

            break;


        default:

            result = 0;

            break;
    }


    /*
     * Zero se utiliza especialmente
     * para BEQ y BNE.
     */

    if (result == 0)

        *zero = 1;

    else

        *zero = 0;


    return result;
}


/* =========================================================
   4. MEMORY
   ========================================================= */

static int valid_address(
    uint32_t address
) {

    /*
     * Las direcciones deben estar
     * alineadas a 4 bytes.
     */

    if (address % 4 != 0)

        return 0;


    if (address / 4 >= MAX_MEMORY_WORDS)

        return 0;


    return 1;
}


/* ---------- READ ---------- */

int memory_read(
    const Memory *memory,
    uint32_t address,
    uint32_t *value
) {

    if (!valid_address(address))

        return -1;


    /*
     * Buscar dirección.
     */

    for (
        size_t i = 0;
        i < memory->count;
        i++
    ) {

        if (
            memory->words[i].address
            == address
        ) {

            *value =
                memory->words[i].value;

            return 0;
        }
    }


    /*
     * Si la dirección existe pero
     * todavía no tiene valor,
     * devolvemos 0.
     */

    *value = 0;


    return 0;
}


/* ---------- WRITE ---------- */

int memory_write(
    Memory *memory,
    uint32_t address,
    uint32_t value
) {

    if (!valid_address(address))

        return -1;


    /*
     * Si la dirección ya existe,
     * actualizarla.
     */

    for (
        size_t i = 0;
        i < memory->count;
        i++
    ) {

        if (
            memory->words[i].address
            == address
        ) {

            memory->words[i].value =
                value;

            return 0;
        }
    }


    /*
     * Crear nueva posición.
     */

    if (
        memory->count
        >= MAX_MEMORY_WORDS
    )

        return -1;


    memory->words[
        memory->count
    ].address = address;


    memory->words[
        memory->count
    ].value = value;


    memory->count++;


    return 0;
}


/* =========================================================
   5. WRITE BACK
   ========================================================= */

void write_back(
    RegisterFile *rf,
    uint8_t reg,
    uint32_t value,
    uint8_t reg_write
) {

    /*
     * RegWrite indica si debemos escribir
     * el resultado en el banco de registros.
     */

    if (
        reg_write &&
        reg != 0 &&
        reg < NUM_REGS
    ) {

        rf->regs[reg] = value;
    }


    /*
     * $zero siempre debe ser 0.
     */

    rf->regs[0] = 0;
}


/* =========================================================
   CONVERSIÓN DE BINARIO A UINT32
   ========================================================= */

static int binary32_to_uint(
    const char *text,
    uint32_t *value
) {

    uint32_t result = 0;

    size_t length =
        strlen(text);


    /*
     * Eliminar salto de línea.
     */

    while (
        length > 0 &&
        (
            text[length - 1] == '\n' ||
            text[length - 1] == '\r' ||
            text[length - 1] == ' ' ||
            text[length - 1] == '\t'
        )
    ) {

        length--;
    }


    /*
     * Debe tener exactamente 32 bits.
     */

    if (length != 32)

        return -1;


    for (
        size_t i = 0;
        i < 32;
        i++
    ) {

        if (
            text[i] != '0' &&
            text[i] != '1'
        ) {

            return -1;
        }


        result =
            (result << 1)
            |
            (text[i] - '0');
    }


    *value = result;


    return 0;
}


/* =========================================================
   LEER PROGRAMA
   ========================================================= */

int load_binary_lines(
    const char *filename,
    uint32_t *buffer,
    size_t max,
    size_t *count
) {

    FILE *file =
        fopen(filename, "r");


    if (!file)

        return -1;


    char line[128];

    size_t n = 0;


    while (
        fgets(
            line,
            sizeof(line),
            file
        )
    ) {

        /*
         * Ignorar líneas vacías.
         */

        if (
            line[0] == '\n' ||
            line[0] == '\r'
        )

            continue;


        if (n >= max) {

            fclose(file);

            return -2;
        }


        if (
            binary32_to_uint(
                line,
                &buffer[n]
            ) != 0
        ) {

            fclose(file);

            return -2;
        }


        n++;
    }


    fclose(file);


    *count = n;


    return 0;
}


/* =========================================================
   LEER BANCO DE REGISTROS
   ========================================================= */

int load_registers(
    const char *filename,
    RegisterFile *rf
) {

    size_t count = 0;


    int result =
        load_binary_lines(
            filename,
            rf->regs,
            NUM_REGS,
            &count
        );


    /*
     * Debemos tener exactamente
     * 32 registros.
     */

    if (
        result != 0 ||
        count != NUM_REGS
    )

        return -1;


    /*
     * $zero siempre es 0.
     */

    rf->regs[0] = 0;


    return 0;
}


/* =========================================================
   LEER MEMORIA
   =========================================================

   Formato:

   dirección valor

   Ejemplo:

   00000000000000000000000001100100
   00000000000000000000000000110010

   representa:

   Memory[100] = 50
*/

int load_memory(
    const char *filename,
    Memory *memory
) {

    FILE *file =
        fopen(filename, "r");


    if (!file)

        return -1;


    memory->count = 0;


    char addressText[64];

    char valueText[64];


    while (
        fscanf(
            file,
            "%63s %63s",
            addressText,
            valueText
        ) == 2
    ) {

        uint32_t address;

        uint32_t value;


        if (
            binary32_to_uint(
                addressText,
                &address
            ) != 0
        ) {

            fclose(file);

            return -2;
        }


        if (
            binary32_to_uint(
                valueText,
                &value
            ) != 0
        ) {

            fclose(file);

            return -2;
        }


        if (
            memory_write(
                memory,
                address,
                value
            ) != 0
        ) {

            fclose(file);

            return -2;
        }
    }


    fclose(file);


    return 0;
}


/* =========================================================
   EJECUTAR PROGRAMA
   ========================================================= */

int execute_program(
    MIPS *cpu,
    size_t max_cycles
) {

    uint32_t pc =
        cpu->pc;


    size_t cycles = 0;


    printf(
        "\n%-8s %-10s %-10s %-10s %-10s %-10s\n",
        "Ciclo",
        "IF",
        "ID",
        "EX",
        "MEM",
        "WB"
    );


    printf(
        "--------------------------------------------------------------\n"
    );


    while (
        cycles < max_cycles
    ) {

        /* =====================================================
           IF
           ===================================================== */

        uint32_t next_pc;


        uint32_t instruction =
            instruction_fetch(
                cpu,
                pc,
                &next_pc
            );


        /*
         * Instrucción 0 = fin.
         */

        if (instruction == 0)

            break;


        /* =====================================================
           ID
           ===================================================== */

        DecodedInstruction decoded;


        if (
            instruction_decode(
                instruction,
                &cpu->rf,
                &decoded
            ) != 0
        ) {

            printf(
                "ERROR: instruccion invalida en PC=%u\n",
                pc
            );

            return -1;
        }


        printf(
            "%-8zu %-10s %-10s %-10s %-10s %-10s\n",
            cycles + 1,
            "FETCH",
            "DECODE",
            "EX",
            "MEM",
            "WB"
        );


        /* =====================================================
           CONTROL
           ===================================================== */

        uint32_t result = 0;

        uint32_t memoryData = 0;

        uint8_t zero = 0;

        uint8_t destination = 0;

        uint8_t regWrite = 0;

        uint8_t memRead = 0;

        uint8_t memWrite = 0;

        uint8_t memToReg = 0;


        /* =====================================================
           INSTRUCCIONES
           ===================================================== */

        switch (
            decoded.opcode
        ) {

            /* ---------------- R TYPE ---------------- */

            case OP_RTYPE:

                /*
                 * JR
                 */

                if (
                    decoded.funct == F_JR
                ) {

                    pc =
                        decoded.readData1;

                    cycles++;

                    continue;
                }


                /*
                 * Operaciones R-Type
                 */

                destination =
                    decoded.rd;

                regWrite = 1;

                break;


            /* ---------------- ADDI ---------------- */

            case OP_ADDI:

                destination =
                    decoded.rt;

                regWrite = 1;

                break;


            /* ---------------- LW ---------------- */

            case OP_LW:

                destination =
                    decoded.rt;

                regWrite = 1;

                memRead = 1;

                memToReg = 1;

                break;


            /* ---------------- SW ---------------- */

            case OP_SW:

                memWrite = 1;

                break;


            /* ---------------- BEQ/BNE ---------------- */

            case OP_BEQ:

            case OP_BNE: {

                /*
                 * Comparar registros mediante resta.
                 */

                uint8_t aluZero;


                alu_execute(
                    decoded.readData1,
                    decoded.readData2,
                    ALU_SUB,
                    &aluZero
                );


                int taken;


                if (
                    decoded.opcode
                    == OP_BEQ
                ) {

                    taken =
                        aluZero;

                } else {

                    taken =
                        !aluZero;
                }


                if (taken) {

                    /*
                     * Branch target:
                     *
                     * PC + 4 +
                     * immediate << 2
                     */

                    pc =
                        next_pc +
                        (
                            (uint32_t)
                            decoded.immediate
                            << 2
                        );

                } else {

                    pc =
                        next_pc;
                }


                cycles++;

                continue;
            }


            /* ---------------- J ---------------- */

            case OP_J:

                /*
                 * Dirección de salto:
                 *
                 * upper 4 bits de PC
                 * +
                 * address << 2
                 */

                pc =
                    (next_pc & 0xF0000000)
                    |
                    (
                        decoded.address
                        << 2
                    );


                cycles++;

                continue;


            default:

                return -1;
        }


        /* =====================================================
           EX - ALU
           ===================================================== */

        uint32_t a = 0;

        uint32_t b = 0;

        uint8_t operation =
            ALU_ADD;


        switch (
            decoded.opcode
        ) {

            case OP_RTYPE:

                a =
                    decoded.readData1;

                b =
                    decoded.readData2;


                switch (
                    decoded.funct
                ) {

                    case F_ADD:

                        operation =
                            ALU_ADD;

                        break;


                    case F_SUB:

                        operation =
                            ALU_SUB;

                        break;


                    case F_AND:

                        operation =
                            ALU_AND;

                        break;


                    case F_OR:

                        operation =
                            ALU_OR;

                        break;


                    case F_NOR:

                        operation =
                            ALU_NOR;

                        break;


                    case F_XOR:

                        operation =
                            ALU_XOR;

                        break;


                    default:

                        return -1;
                }

                break;


            case OP_ADDI:

                a =
                    decoded.readData1;

                b =
                    (uint32_t)
                    decoded.immediate;

                operation =
                    ALU_ADD;

                break;


            case OP_LW:

            case OP_SW:

                /*
                 * Dirección =
                 * registro base + offset
                 */

                a =
                    decoded.readData1;

                b =
                    (uint32_t)
                    decoded.immediate;

                operation =
                    ALU_ADD;

                break;


            default:

                break;
        }


        result =
            alu_execute(
                a,
                b,
                operation,
                &zero
            );


        /* =====================================================
           MEM
           ===================================================== */

        if (memWrite) {

            if (
                memory_write(
                    &cpu->memory,
                    result,
                    decoded.readData2
                ) != 0
            ) {

                printf(
                    "ERROR: direccion de memoria invalida\n"
                );

                return -1;
            }
        }


        if (memRead) {

            if (
                memory_read(
                    &cpu->memory,
                    result,
                    &memoryData
                ) != 0
            ) {

                printf(
                    "ERROR: direccion de memoria invalida\n"
                );

                return -1;
            }
        }


        /* =====================================================
           WB
           ===================================================== */

        if (regWrite) {

            if (memToReg)

                write_back(
                    &cpu->rf,
                    destination,
                    memoryData,
                    1
                );

            else

                write_back(
                    &cpu->rf,
                    destination,
                    result,
                    1
                );
        }


        pc =
            next_pc;


        cycles++;
    }


    cpu->pc =
        pc;


    return 0;
}


/* =========================================================
   MOSTRAR REGISTROS
   ========================================================= */

void print_registers(
    const RegisterFile *rf
) {

    printf(
        "\nBANCO DE REGISTROS\n"
    );


    printf(
        "-----------------------------\n"
    );


    for (
        int i = 0;
        i < NUM_REGS;
        i++
    ) {

        printf(
            "$%02d = %10u = 0x%08X\n",
            i,
            rf->regs[i],
            rf->regs[i]
        );
    }
}


/* =========================================================
   FUNCIONES DE ENCODING
   ========================================================= */

uint32_t enc_r(
    uint8_t rs,
    uint8_t rt,
    uint8_t rd,
    uint8_t funct
) {

    return
        ((uint32_t)rs << 21)
        |
        ((uint32_t)rt << 16)
        |
        ((uint32_t)rd << 11)
        |
        funct;
}


uint32_t enc_i(
    uint8_t opcode,
    uint8_t rs,
    uint8_t rt,
    int16_t immediate
) {

    return
        ((uint32_t)opcode << 26)
        |
        ((uint32_t)rs << 21)
        |
        ((uint32_t)rt << 16)
        |
        ((uint16_t)immediate);
}


uint32_t enc_j(
    uint8_t opcode,
    uint32_t address
) {

    return
        ((uint32_t)opcode << 26)
        |
        (address & 0x03FFFFFF);
}