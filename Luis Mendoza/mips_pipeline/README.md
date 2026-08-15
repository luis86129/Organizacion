# Simulador MIPS de 32 bits

Simulador básico de un procesador MIPS de 32 bits desarrollado en lenguaje C.

El proyecto implementa las principales etapas de procesamiento de una instrucción:

1. Instruction Fetch + Program Counter
2. Instruction Decode
3. ALU
4. Memory
5. Write Back

También incluye pruebas unitarias para cada módulo y pruebas integrales mediante vectores de prueba.

---

## 1. Características

El simulador implementa las siguientes instrucciones MIPS:

### Instrucciones R-Type

- `add`
- `sub`
- `and`
- `or`
- `nor`
- `xor`
- `jr`

### Instrucciones I-Type

- `addi`
- `lw`
- `sw`
- `beq`
- `bne`

### Instrucciones J-Type

- `j`

Todas las instrucciones utilizan una representación de 32 bits.

---

# 2. Arquitectura

El procesamiento de una instrucción se divide en cinco etapas:

```text
                    ┌───────────────┐
                    │      PC       │
                    └───────┬───────┘
                            │
                            ▼
                  ┌───────────────────┐
                  │ 1. Instruction    │
                  │    Fetch + PC     │
                  └─────────┬─────────┘
                            │
                            ▼
                  ┌───────────────────┐
                  │ 2. Instruction    │
                  │    Decode         │
                  └─────────┬─────────┘
                            │
                            ▼
                  ┌───────────────────┐
                  │ 3. ALU            │
                  │    Execute        │
                  └─────────┬─────────┘
                            │
                            ▼
                  ┌───────────────────┐
                  │ 4. Memory         │
                  │    Read / Write   │
                  └─────────┬─────────┘
                            │
                            ▼
                  ┌───────────────────┐
                  │ 5. Write Back     │
                  └─────────┬─────────┘
                            │
                            ▼
                     Register File