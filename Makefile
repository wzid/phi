# Compiler settings
CC = clang
CFLAGS = -Wall -Wextra $(shell llvm-config --cflags)
LDFLAGS = $(shell llvm-config --ldflags --libs core)

# Directory structure
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Find source files and generate object file paths
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Output executable
TARGET = $(BIN_DIR)/mycompiler

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I./include -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)