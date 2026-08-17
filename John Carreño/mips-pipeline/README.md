# Simulador MIPS con pipeline de 5 etapas

Implementación en C del simulador planificado en el Proyecto Parte 1. Las cinco
etapas son funciones puras: reciben su registro de pipeline por parámetro y
devuelven una estructura de salida, sin variables globales. Todo el estado vive
en `InstrMem`, `DataMem`, `RegFile` y `Pipeline`.

## Instrucciones soportadas

`add` `sub` `addi` `and` `or` `nor` `xor` `lw` `sw` `beq` `bne` `j` `jr`

## Estructura

```
include/mips.h        tipos, constantes de la ISA y prototipos
include/encode.h      ensamblador mínimo (arma las palabras de 32 bits)
src/memory.c          carga/volcado de imem, dmem y banco de registros (.bin)
src/stage_if.c        etapa 1: Instruction Fetch + Program Counter
src/stage_id.c        etapa 2: Instruction Decode (unidad de control)
src/stage_ex.c        etapa 3: ALU y resolución de saltos
src/stage_mem.c       etapa 4: acceso a memoria de datos
src/stage_wb.c        etapa 5: Write Back
src/pipeline.c        ciclo, detección de riesgos, burbujas y flush
src/main.c            programa principal (CLI)
tests/test_*.c        un unit test por etapa
tests/test_integration.c  test del sistema con los vectores S1..S8
tests/programs.c      definición de los ocho vectores de prueba
tools/vecgen.c        genera los .bin de los vectores
docs/diagramas.tex    diagramas del sistema y de subfunciones
```

## Formato de los archivos .bin

Los tres archivos contienen **palabras de 32 bits en little-endian**, sin
cabecera. La lectura se hace byte por byte, así que no depende del endianness
de la máquina.

| Archivo | Contenido |
|---|---|
| `imem.bin` | una palabra por instrucción, en orden desde la dirección 0 |
| `dmem.bin` | contenido inicial de la memoria de datos, palabra 0 = dirección 0 |
| `regs.bin` | 32 palabras con el valor inicial de `$0..$31` (`$0` se fuerza a 0) |

## Compilar y probar

```bash
make            # compila build/mips_sim
make test       # corre los 5 unit tests y el test integral
make vectors    # genera vectors/*.bin
make run        # corre el simulador sobre el vector S8
make docs       # compila docs/diagramas.tex
make clean
```

Ejemplo de uso directo:

```bash
./build/mips_sim --imem vectors/s8_imem.bin \
                 --dmem vectors/s8_dmem.bin \
                 --out-regs salida_regs.bin --trace
```

## Decisiones de diseño

- **Orden inverso de las etapas.** El ciclo llama a WB, MEM, EX, ID e IF en ese
  orden. Si se llamaran en orden directo actualizando el estado sobre la marcha,
  una instrucción atravesaría las cinco etapas en un solo ciclo.
- **WB escribe antes de que ID lea.** Equivale a escribir en la primera mitad
  del ciclo y leer en la segunda, así que una dependencia a distancia 3 no
  necesita burbuja.
- **Sin forwarding.** Los riesgos de datos se resuelven con burbujas: se compara
  el registro que ID va a leer contra los destinos de las instrucciones que están
  en EX y en MEM.
- **Saltos resueltos en EX**, reutilizando la resta de la ALU para `beq`/`bne`.
  Cuesta dos ciclos de penalidad: al tomarse el salto se descarta lo que quedó
  en IF/ID y en ID/EX.
- **Errores en lugar de caídas.** Instrucción ilegal, dirección desalineada y
  dirección fuera de rango detienen el simulador con un código de error; nunca
  se indexa fuera del arreglo ni se ejecuta un `add` fantasma.
- **`$0` protegido** en la escritura del banco de registros.

## Vectores de prueba

| # | Programa | Qué verifica | Resultado esperado |
|---|---|---|---|
| S1 | `addi`, `add`, `sub` | camino aritmético | `$3=13`, `$4=7` |
| S2 | `and`, `or`, `xor`, `nor` | operaciones lógicas | `$6=0xFFFFFF00` |
| S3 | `sw` + `lw` | ida y vuelta a memoria | `$3=42` |
| S4 | `lw` + uso inmediato | riesgo load-use | `$4=14`, con burbujas |
| S5 | `beq` tomado y no tomado | salto y flush | `$3=$4=0`, `$5=7`, `$6=3` |
| S6 | `bne` en sus dos casos | inversión del flag zero | `$3=0`, `$4=1`, `$5=2` |
| S7 | `j` y `jr` | destino absoluto y retorno | `$2=$3=0`, `$4=7` |
| S8 | bucle + suma de arreglo | programa integrador | `$1=100`, `dmem[8]=100` |

El test integral agrega tres casos de robustez: escritura a `$0`, instrucción
ilegal y acceso a memoria fuera de rango.
