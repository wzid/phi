# Compiler settings
CC := clang
LLVM_CONFIG := llvm-config
# Compiler flags w/ LLVM flags
CFLAGS := -Wall -Wextra $(shell $(LLVM_CONFIG) --cflags)
# Linker flags for LLVM
LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs core)

# Directory structure
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
OUTPUT_DIR := output

# Automatically find all source files and generate object file paths
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)           # Find all .c files in src directory
# Convert to .o paths
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Output file paths
COMPILER := $(BIN_DIR)/mycompiler             # The compiler executable
OUTPUT_BC := $(OUTPUT_DIR)/output.bc          # LLVM bitcode output
OUTPUT_LL := $(OUTPUT_DIR)/output.ll          # Human-readable LLVM IR
OUTPUT_PROGRAM := $(OUTPUT_DIR)/program       # Final executable program

# Phony targets - Ensures when we run `make clean` it always runs regardless 
# if there is a file named `clean` or not
.PHONY: all clean run test

# Default target: build the compiler
all: $(COMPILER)

# Link object files
$(COMPILER): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile .c files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I./include -c $< -o $@

# Create directories if they don't exist
$(BIN_DIR) $(OBJ_DIR) $(OUTPUT_DIR):
	mkdir -p $@

# Generate LLVM bitcode by running the compiler
$(OUTPUT_BC): $(COMPILER) | $(OUTPUT_DIR)
# Run the compiler
	./$(COMPILER)
	mv output.bc $(OUTPUT_BC)

# Convert bitcode to human-readable LLVM IR
$(OUTPUT_LL): $(OUTPUT_BC)
	llvm-dis $(OUTPUT_BC) -o $(OUTPUT_LL)

# Compile LLVM IR to an executable program
$(OUTPUT_PROGRAM): $(OUTPUT_LL)
	$(CC) -Wno-override-module $(OUTPUT_LL) -o $(OUTPUT_PROGRAM)

# Run the full pipeline and execute the resulting program
run: $(OUTPUT_PROGRAM)
	./$(OUTPUT_PROGRAM)

# Clean everything and rebuild from scratch, then run
test: clean all run

# Remove all generated files and directories
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(OUTPUT_DIR)