# Simulador MIPS con Pipeline (Parte 1)

Implementación en C del simulador MIPS planificado en Proyecto Parte 1:
5 funciones correspondientes a las etapas del pipeline clásico, cada una
con sus unit tests, más un test de sistema con vectores de prueba.

## Instrucciones soportadas

`add, sub, addi, and, or, nor, xor, lw, sw, beq, bne, j, jr`

## Estructura del proyecto

```
mips_sim/
├── include/
│   ├── mips_sim.h      # structs, enums y prototipos de las 5 etapas
│   └── mini_asm.h       # ensamblador mínimo, solo para escribir tests legibles
├── src/
│   ├── fetch.c          # Etapa 1: IF + Program Counter
│   ├── decode.c          # Etapa 2: Instruction Decode
│   ├── alu.c              # Etapa 3: ALU (Execute)
│   ├── memory.c            # Etapa 4: Memory
│   ├── writeback.c          # Etapa 5: Write Back
│   ├── cpu.c                 # Une las 5 etapas para ejecutar una instrucción completa
│   └── main.c                 # Programa de demostración
├── test/
│   ├── test_common.h    # framework de testing casero (assert + pass/fail)
│   ├── test_fetch.c
│   ├── test_decode.c
│   ├── test_alu.c
│   ├── test_memory.c
│   ├── test_writeback.c
│   └── test_system.c    # test de sistema: programa con las 13 instrucciones
├── diagrams/             # diagramas de Proyecto Parte 1 (sistema y subfunciones)
└── Makefile
```

## Compilar y correr

```bash
make          # compila el simulador (build/mips_sim)
make run      # compila y corre el programa de demostración
make test     # compila y corre TODOS los unit tests + el test de sistema
make clean    # borra los binarios generados
```

## Decisiones de diseño

- **Sin solapamiento de etapas (no pipeline real, no hazards).** Cada
  instrucción atraviesa las 5 etapas por completo antes de que la
  siguiente comience.
- **Centinela de fin de programa:** la palabra `0x00000000` se usa como
  instrucción `HALT`. `cpu_step()` la detecta y detiene la ejecución.
- **Validación de memoria:** `mem_read`/`mem_write` verifican que la
  dirección esté dentro del rango de `DATA_MEM_WORDS`; si no, el programa
  termina con un mensaje de error explícito en vez de leer/escribir fuera
  de rango.
- **Protección de `$0`:** `register_write()` ignora cualquier intento de
  escribir en el registro 0, sin importar la señal `RegWrite`.
- **`mini_asm.h`** no es parte del simulador: es solo una utilidad de
  testing para armar instrucciones de 32 bits legibles (`ASM_ADD(rd,rs,rt)`,
  etc.) en vez de escribir hexadecimal a mano en los programas de prueba.

## Cobertura de tests

| Módulo | Unit tests | Casos límite incluidos |
|---|---|---|
| fetch | 5 | último índice válido de memoria de instrucciones |
| decode | 10 | overflow de signo en `sign_extend`, tipo R/I/J |
| alu | 5 | overflow aritmético con enteros con signo |
| memory | 5 | última dirección válida, passthrough sin acceso a memoria |
| write_back | 6 | protección de `$0` |
| sistema | 2 | programa completo (13 instrucciones, beq/bne tomados, j, jr), escritura a `$0` |
