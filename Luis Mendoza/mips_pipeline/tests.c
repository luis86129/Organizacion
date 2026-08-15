#include "mips.h"

#include <stdio.h>
#include <string.h>


/*
 * Contadores de pruebas.
 */

int passed = 0;
int failed = 0;


/* =========================================================
   MACROS
   ========================================================= */

#define ASSERT_UINT(name, obtained, expected)             \
    do {                                                  \
        if ((obtained) == (expected)) {                   \
            printf("[PASS] %s\n", name);                 \
            passed++;                                     \
        } else {                                          \
            printf("[FAIL] %s\n", name);                \
            printf("       Obtenido: %u\n",              \
                   (unsigned)(obtained));                \
            printf("       Esperado: %u\n",              \
                   (unsigned)(expected));                \
            failed++;                                     \
        }                                                   \
    } while (0)


#define ASSERT_INT(name, obtained, expected)              \
    do {                                                  \
        if ((obtained) == (expected)) {                   \
            printf("[PASS] %s\n", name);                 \
            passed++;                                     \
        } else {                                          \
            printf("[FAIL] %s\n", name);                \
            printf("       Obtenido: %d\n",              \
                   (int)(obtained));                      \
            printf("       Esperado: %d\n",              \
                   (int)(expected));                      \
            failed++;                                     \
        }                                                   \
    } while (0)


/* =========================================================
   UNIT TEST - IF
   ========================================================= */

void test_IF(void) {

    printf(
        "\n===== TEST IF =====\n"
    );


    MIPS cpu = {0};


    /*
     * Colocar una instrucción
     * artificialmente.
     */

    cpu.program[0] =
        0x12345678;


    cpu.programSize = 1;


    uint32_t next_pc;


    uint32_t instruction =
        instruction_fetch(
            &cpu,
            0,
            &next_pc
        );


    ASSERT_UINT(
        "IF obtiene instruccion",
        instruction,
        0x12345678
    );


    ASSERT_UINT(
        "IF incrementa PC",
        next_pc,
        4
    );
}


/* =========================================================
   UNIT TEST - ID
   ========================================================= */

void test_ID(void) {

    printf(
        "\n===== TEST ID =====\n"
    );


    RegisterFile rf = {0};


    /*
     * $1 = 10
     * $2 = 20
     */

    rf.regs[1] = 10;
    rf.regs[2] = 20;


    /*
     * add $3,$1,$2
     */

    uint32_t instruction =
        enc_r(
            1,
            2,
            3,
            0x20
        );


    DecodedInstruction decoded;


    int result =
        instruction_decode(
            instruction,
            &rf,
            &decoded
        );


    ASSERT_INT(
        "ID decodifica ADD",
        result,
        0
    );


    ASSERT_INT(
        "ID obtiene RS",
        decoded.rs,
        1
    );


    ASSERT_INT(
        "ID obtiene RT",
        decoded.rt,
        2
    );


    ASSERT_INT(
        "ID obtiene RD",
        decoded.rd,
        3
    );


    ASSERT_UINT(
        "ID lee registro RS",
        decoded.readData1,
        10
    );


    ASSERT_UINT(
        "ID lee registro RT",
        decoded.readData2,
        20
    );


    /*
     * ADDI con inmediato negativo.
     *
     * addi $3,$1,-5
     */

    instruction =
        enc_i(
            0x08,
            1,
            3,
            -5
        );


    result =
        instruction_decode(
            instruction,
            &rf,
            &decoded
        );


    ASSERT_INT(
        "ID decodifica ADDI",
        result,
        0
    );


    ASSERT_INT(
        "ID extension de signo",
        decoded.immediate,
        -5
    );
}


/* =========================================================
   UNIT TEST - ALU
   ========================================================= */

void test_ALU(void) {

    printf(
        "\n===== TEST ALU =====\n"
    );


    uint8_t zero;


    /*
     * ADD
     */

    ASSERT_UINT(
        "ALU ADD",
        alu_execute(
            10,
            5,
            0,
            &zero
        ),
        15
    );


    /*
     * SUB
     */

    ASSERT_UINT(
        "ALU SUB",
        alu_execute(
            10,
            5,
            1,
            &zero
        ),
        5
    );


    /*
     * AND
     */

    ASSERT_UINT(
        "ALU AND",
        alu_execute(
            0xA,
            0xC,
            2,
            &zero
        ),
        0x8
    );


    /*
     * OR
     */

    ASSERT_UINT(
        "ALU OR",
        alu_execute(
            0xA,
            0xC,
            3,
            &zero
        ),
        0xE
    );


    /*
     * NOR
     */

    ASSERT_UINT(
        "ALU NOR",
        alu_execute(
            0xA,
            0xC,
            4,
            &zero
        ),
        ~0xE
    );


    /*
     * XOR
     */

    ASSERT_UINT(
        "ALU XOR",
        alu_execute(
            0xA,
            0xC,
            5,
            &zero
        ),
        0x6
    );


    /*
     * ZERO FLAG
     */

    alu_execute(
        20,
        20,
        1,
        &zero
    );


    ASSERT_INT(
        "ALU ZERO",
        zero,
        1
    );
}


/* =========================================================
   UNIT TEST - MEMORY
   ========================================================= */

void test_MEMORY(void) {

    printf(
        "\n===== TEST MEMORY =====\n"
    );


    Memory memory = {0};


    uint32_t value;


    /*
     * Escribir:
     *
     * Memory[100] = 50
     */

    int result =
        memory_write(
            &memory,
            100,
            50
        );


    ASSERT_INT(
        "MEM WRITE",
        result,
        0
    );


    /*
     * Leer memoria.
     */

    result =
        memory_read(
            &memory,
            100,
            &value
        );


    ASSERT_INT(
        "MEM READ",
        result,
        0
    );


    ASSERT_UINT(
        "MEM valor",
        value,
        50
    );


    /*
     * Dirección inválida.
     */

    result =
        memory_write(
            &memory,
            2,
            100
        );


    ASSERT_INT(
        "MEM direccion invalida",
        result,
        -1
    );
}


/* =========================================================
   UNIT TEST - WRITE BACK
   ========================================================= */

void test_WB(void) {

    printf(
        "\n===== TEST WRITE BACK =====\n"
    );


    RegisterFile rf = {0};


    /*
     * Escribir 100 en $5.
     */

    write_back(
        &rf,
        5,
        100,
        1
    );


    ASSERT_UINT(
        "WB escribe registro",
        rf.regs[5],
        100
    );


    /*
     * Intentar modificar $zero.
     */

    write_back(
        &rf,
        0,
        999,
        1
    );


    ASSERT_UINT(
        "WB protege $zero",
        rf.regs[0],
        0
    );


    /*
     * RegWrite = 0
     */

    write_back(
        &rf,
        6,
        500,
        0
    );


    ASSERT_UINT(
        "WB no escribe con RegWrite=0",
        rf.regs[6],
        0
    );
}


/* =========================================================
   TEST INTEGRAL 1
   ========================================================= */

void test_integral_ADD(void) {

    printf(
        "\n===== TEST INTEGRAL 1 =====\n"
    );


    MIPS cpu = {0};


    /*
     * addi $1,$0,10
     *
     * addi $2,$0,20
     *
     * add $3,$1,$2
     */


    cpu.program[0] =
        enc_i(
            0x08,
            0,
            1,
            10
        );


    cpu.program[1] =
        enc_i(
            0x08,
            0,
            2,
            20
        );


    cpu.program[2] =
        enc_r(
            1,
            2,
            3,
            0x20
        );


    cpu.programSize = 3;


    /*
     * Ejecutar.
     */

    int result =
        execute_program(
            &cpu,
            20
        );


    ASSERT_INT(
        "Sistema ADD ejecuta",
        result,
        0
    );


    ASSERT_UINT(
        "Sistema resultado ADD",
        cpu.rf.regs[3],
        30
    );
}


/* =========================================================
   TEST INTEGRAL 2 - LW/SW
   ========================================================= */

void test_integral_memory(void) {

    printf(
        "\n===== TEST INTEGRAL 2 =====\n"
    );


    MIPS cpu = {0};


    /*
     * addi $1,$0,100
     *
     * addi $2,$0,50
     *
     * sw $2,0($1)
     *
     * lw $3,0($1)
     */


    cpu.program[0] =
        enc_i(
            0x08,
            0,
            1,
            100
        );


    cpu.program[1] =
        enc_i(
            0x08,
            0,
            2,
            50
        );


    cpu.program[2] =
        enc_i(
            0x2B,
            1,
            2,
            0
        );


    cpu.program[3] =
        enc_i(
            0x23,
            1,
            3,
            0
        );


    cpu.programSize = 4;


    int result =
        execute_program(
            &cpu,
            20
        );


    ASSERT_INT(
        "Sistema LW/SW ejecuta",
        result,
        0
    );


    ASSERT_UINT(
        "Sistema LW recupera valor",
        cpu.rf.regs[3],
        50
    );
}


/* =========================================================
   TEST INTEGRAL 3 - BEQ
   ========================================================= */

void test_integral_BEQ(void) {

    printf(
        "\n===== TEST INTEGRAL 3 =====\n"
    );


    MIPS cpu = {0};


    /*
     *
     * addi $1,$0,7
     *
     * addi $2,$0,7
     *
     * beq $1,$2,1
     *
     * addi $3,$0,99
     *
     * addi $3,$0,42
     *
     */


    cpu.program[0] =
        enc_i(
            0x08,
            0,
            1,
            7
        );


    cpu.program[1] =
        enc_i(
            0x08,
            0,
            2,
            7
        );


    cpu.program[2] =
        enc_i(
            0x04,
            1,
            2,
            1
        );


    cpu.program[3] =
        enc_i(
            0x08,
            0,
            3,
            99
        );


    cpu.program[4] =
        enc_i(
            0x08,
            0,
            3,
            42
        );


    cpu.programSize = 5;


    int result =
        execute_program(
            &cpu,
            20
        );


    ASSERT_INT(
        "Sistema BEQ ejecuta",
        result,
        0
    );


    /*
     * Debemos terminar con 42.
     */

    ASSERT_UINT(
        "Sistema BEQ resultado",
        cpu.rf.regs[3],
        42
    );
}


/* =========================================================
   MAIN DE TESTS
   ========================================================= */

int main(void) {

    printf(
        "\n========================================\n"
        "       TESTS DEL SIMULADOR MIPS\n"
        "========================================\n"
    );


    /*
     * UNIT TESTS
     */

    test_IF();

    test_ID();

    test_ALU();

    test_MEMORY();

    test_WB();


    /*
     * TESTS INTEGRALES
     */

    test_integral_ADD();

    test_integral_memory();

    test_integral_BEQ();


    /*
     * RESULTADO FINAL
     */

    printf(
        "\n========================================\n"
    );


    printf(
        "Tests aprobados: %d\n",
        passed
    );


    printf(
        "Tests fallidos : %d\n",
        failed
    );


    printf(
        "========================================\n"
    );


    if (failed == 0) {

        printf(
            "TODOS LOS TESTS PASARON\n"
        );

        return 0;
    }


    printf(
        "EXISTEN TESTS FALLIDOS\n"
    );


    return 1;
}