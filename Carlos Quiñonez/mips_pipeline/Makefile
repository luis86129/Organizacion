CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

# Fuentes del simulador (sin main.c, para poder reusarlas en los tests)
SIM_SRCS = $(SRC_DIR)/fetch.c $(SRC_DIR)/decode.c $(SRC_DIR)/alu.c \
           $(SRC_DIR)/memory.c $(SRC_DIR)/writeback.c $(SRC_DIR)/cpu.c

.PHONY: all clean test run

all: $(BUILD_DIR)/mips_sim

# ---- Programa principal ----
$(BUILD_DIR)/mips_sim: $(SRC_DIR)/main.c $(SIM_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/main.c $(SIM_SRCS)

run: $(BUILD_DIR)/mips_sim
	./$(BUILD_DIR)/mips_sim

# ---- Unit tests (uno por etapa) + test de sistema ----
test: $(BUILD_DIR)/test_fetch $(BUILD_DIR)/test_decode $(BUILD_DIR)/test_alu \
      $(BUILD_DIR)/test_memory $(BUILD_DIR)/test_writeback $(BUILD_DIR)/test_system
	@echo "--------------------------------------"
	@echo "Ejecutando todos los tests:"
	@echo "--------------------------------------"
	@./$(BUILD_DIR)/test_fetch
	@./$(BUILD_DIR)/test_decode
	@./$(BUILD_DIR)/test_alu
	@./$(BUILD_DIR)/test_memory
	@./$(BUILD_DIR)/test_writeback
	@./$(BUILD_DIR)/test_system

$(BUILD_DIR)/test_fetch: $(TEST_DIR)/test_fetch.c $(SRC_DIR)/fetch.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_fetch.c $(SRC_DIR)/fetch.c

$(BUILD_DIR)/test_decode: $(TEST_DIR)/test_decode.c $(SRC_DIR)/decode.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_decode.c $(SRC_DIR)/decode.c

$(BUILD_DIR)/test_alu: $(TEST_DIR)/test_alu.c $(SRC_DIR)/alu.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_alu.c $(SRC_DIR)/alu.c

$(BUILD_DIR)/test_memory: $(TEST_DIR)/test_memory.c $(SRC_DIR)/memory.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_memory.c $(SRC_DIR)/memory.c

$(BUILD_DIR)/test_writeback: $(TEST_DIR)/test_writeback.c $(SRC_DIR)/writeback.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_writeback.c $(SRC_DIR)/writeback.c

$(BUILD_DIR)/test_system: $(TEST_DIR)/test_system.c $(SIM_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_DIR)/test_system.c $(SIM_SRCS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
