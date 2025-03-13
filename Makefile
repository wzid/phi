CC := clang
LLVM_CONFIG := llvm-config
CFLAGS := -Wall -Wextra $(shell $(LLVM_CONFIG) --cflags)
LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs core)
INCLUDES := -I./include

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
OUTPUT_DIR := output

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

COMPILER := $(BIN_DIR)/mycompiler
OUTPUT_BC := $(OUTPUT_DIR)/output.bc
OUTPUT_LL := $(OUTPUT_DIR)/output.ll
OUTPUT_PROGRAM := $(OUTPUT_DIR)/program

.PHONY: all clean run test

all: $(COMPILER)

$(COMPILER): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR) $(OUTPUT_DIR):
	mkdir -p $@

# Rule to generate LLVM bitcode
$(OUTPUT_BC): $(COMPILER) | $(OUTPUT_DIR)
	./$(COMPILER)
	mv output.bc $(OUTPUT_BC)

# Rule to convert bitcode to readable LLVM IR
$(OUTPUT_LL): $(OUTPUT_BC)
	llvm-dis $(OUTPUT_BC) -o $(OUTPUT_LL)

# Rule to compile LLVM IR to an executable
$(OUTPUT_PROGRAM): $(OUTPUT_LL)
	$(CC) -Wno-override-module $(OUTPUT_LL) -o $(OUTPUT_PROGRAM)

# Run the compiler, generate bitcode, and compile+run the output
run: $(OUTPUT_PROGRAM)
	./$(OUTPUT_PROGRAM)

# Rule that demonstrates the full workflow
test: clean all run

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(OUTPUT_DIR)