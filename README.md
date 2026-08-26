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
src/memory.c          carga/volcado de imem, dmem y banco de registros (.txt)
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
tools/vecgen.c        genera los .txt de los vectores
docs/diagramas.tex    fuente LaTeX de los diagramas (por si hay que editarla)
docs/diagramas.pdf    diagramas ya compilados (el entregable)
```

## Formato de los archivos .txt

Los tres archivos son **texto plano con una palabra de 32 bits por línea, escrita
en binario**: 32 caracteres `0`/`1`, del bit 31 al bit 0. Es lo que se ve al abrir
cualquiera de ellos y es lo que escribe el simulador (`--out-regs`, `--out-dmem`,
`make vectors`).

```
00100000000000010000000000000000    # addi $1, $0, 0
10001100010001000000000000000000    # lw   $4, 0($2)
```

Además del binario, el lector tolera, para archivos escritos a mano:

- hexadecimal, con o sin prefijo: `20010005` o `0x20010005`
- líneas en blanco y comentarios que empiecen con `#` o `//`
- más de una palabra por línea, separadas por espacios

| Archivo | Contenido |
|---|---|
| `imem.txt` | una palabra por instrucción, en orden desde la dirección 0 |
| `dmem.txt` | contenido inicial de la memoria de datos, palabra 0 = dirección 0 |
| `regs.txt` | 32 palabras con el valor inicial de `$0..$31` (`$0` se fuerza a 0) |

## Compilar y probar

```bash
make            # compila build/mips_sim
make test       # corre los 5 unit tests y el test integral
make vectors    # genera vectors/*.txt
make run        # corre el simulador sobre el vector S8
make clean
```

Los diagramas (`docs/diagramas.pdf`) ya vienen compilados en el repo; no hace
falta tener LaTeX instalado para revisarlos. Si necesitas editarlos, la fuente
está en `docs/diagramas.tex` y se recompila con `pdflatex diagramas.tex` desde
esa carpeta.

Ejemplo de uso directo:

```bash
./build/mips_sim --imem vectors/s8_imem.txt \
                 --dmem vectors/s8_dmem.txt \
                 --out-regs salida_regs.txt --trace
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

## Sobre la validación con un input externo

Si la profesora carga su propio `imem.txt` (con o sin `dmem.txt`/`regs.txt`) y
compara el estado final, el resultado va a coincidir con cualquier otra
implementación correcta de las 13 instrucciones, porque el simulador sigue la
semántica estándar de MIPS documentada en la Parte 1: misma codificación de
opcodes/funct, mismo cálculo de destinos de salto
(`PC+4 + (imm << 2)` para `beq`/`bne`, `(PC+4)[31:28] | (addr << 2)` para `j`),
misma extensión de signo del inmediato y mismo protegido de `$0`. El resultado
final (registros y memoria) no depende de cómo esté organizado el código
interno, solo de la ISA.

Dos puntos que sí conviene tener presentes porque son decisiones de diseño, no
"bugs", y podrían no coincidir con otra implementación si no se tomó la misma
decisión:

- **Overflow con signo en `add`/`addi`/`sub`.** El MIPS real dispara una
  excepción; este simulador, por restricción declarada en la Parte 1
  ("sin excepciones ni traps de overflow"), deja que el resultado envuelva
  (`0x7FFFFFFF + 1 = 0x80000000`) y sigue corriendo. Si el vector de la
  profesora fuerza un overflow y su referencia sí lo atrapa, el resultado va a
  diferir ahí. Está cubierto en `tests/test_ex.c`.
- **Capacidad de las memorias.** `IMEM_WORDS` y `DMEM_WORDS` están en 1024
  palabras (4 KB) cada una, en `include/mips.h`. Un programa o una dirección
  que se pase de ese límite da `MIPS_ERR_RANGE` en vez de un resultado.

Para reproducir exactamente lo que ella probablemente va a hacer:

```bash
make                     # build/mips_sim
./build/mips_sim --imem <su_archivo.txt> [--dmem <otro.txt>] \
                 --out-regs salida_regs.txt --out-dmem salida_dmem.txt
```

`salida_regs.txt` y `salida_dmem.txt` quedan en el mismo formato binario de 32
caracteres por línea, así que son directamente comparables (por ejemplo con
`diff`) contra el resultado esperado o el de otro grupo.

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
