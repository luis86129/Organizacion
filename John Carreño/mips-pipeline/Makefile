# =====================================================================
# Makefile -- Simulador MIPS con pipeline de 5 etapas
#
#   make            compila el simulador
#   make test       compila y corre los 5 unit tests + el test integral
#   make vectors    genera los .bin de los vectores de prueba S1..S8
#   make run        corre el simulador sobre el vector S8
#   make docs       compila los diagramas (docs/diagramas.tex -> PDF)
#   make clean      borra binarios, objetos y vectores generados
# =====================================================================

CC       := gcc
CFLAGS   := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude -Itests
LDFLAGS  :=

BUILD    := build
SRC_DIR  := src
TEST_DIR := tests
TOOL_DIR := tools

# Nucleo del simulador: las 5 etapas + memoria + pipeline
CORE_SRC := $(SRC_DIR)/memory.c   \
            $(SRC_DIR)/stage_if.c \
            $(SRC_DIR)/stage_id.c \
            $(SRC_DIR)/stage_ex.c \
            $(SRC_DIR)/stage_mem.c \
            $(SRC_DIR)/stage_wb.c \
            $(SRC_DIR)/pipeline.c

CORE_OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(CORE_SRC))

SIM      := $(BUILD)/mips_sim
VECGEN   := $(BUILD)/vecgen

UNIT     := test_if test_id test_ex test_mem test_wb
UNIT_BIN := $(addprefix $(BUILD)/,$(UNIT))
INTEG    := $(BUILD)/test_integration

.PHONY: all test vectors run docs clean

all: $(SIM)

# --------------------------------------------------------------------
# Compilacion
# --------------------------------------------------------------------
$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/programs.o: $(TEST_DIR)/programs.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(SIM): $(CORE_OBJ) $(BUILD)/main.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Un binario por unit test
$(BUILD)/test_%: $(TEST_DIR)/test_%.c $(CORE_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $< $(CORE_OBJ) -o $@ $(LDFLAGS)

# El test integral ademas necesita los programas de prueba
$(INTEG): $(TEST_DIR)/test_integration.c $(CORE_OBJ) $(BUILD)/programs.o | $(BUILD)
	$(CC) $(CFLAGS) $< $(CORE_OBJ) $(BUILD)/programs.o -o $@ $(LDFLAGS)

$(VECGEN): $(TOOL_DIR)/vecgen.c $(CORE_OBJ) $(BUILD)/programs.o | $(BUILD)
	$(CC) $(CFLAGS) $< $(CORE_OBJ) $(BUILD)/programs.o -o $@ $(LDFLAGS)

# --------------------------------------------------------------------
# Pruebas: los 5 unit tests y despues el test integral.
# Cada binario devuelve 0 solo si todas sus comprobaciones pasaron.
# --------------------------------------------------------------------
test: $(UNIT_BIN) $(INTEG)
	@echo ""
	@for t in $(UNIT_BIN) $(INTEG); do \
		./$$t || exit 1; \
	done
	@echo "Todas las pruebas pasaron."

# --------------------------------------------------------------------
# Vectores de prueba en formato .bin
# --------------------------------------------------------------------
vectors: $(VECGEN)
	@mkdir -p vectors
	@./$(VECGEN)

run: $(SIM) vectors
	./$(SIM) --imem vectors/s8_imem.bin --dmem vectors/s8_dmem.bin

# --------------------------------------------------------------------
# Diagramas
# --------------------------------------------------------------------
docs:
	cd docs && pdflatex -interaction=nonstopmode diagramas.tex

clean:
	rm -rf $(BUILD) vectors/*.bin
	rm -f docs/*.aux docs/*.log docs/*.pdf
