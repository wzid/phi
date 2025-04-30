# --- toolchain --------------------------------------------------------------
CC      := clang
CFLAGS  := -Wall -Wextra $(shell llvm-config --cflags) -I./include
LDFLAGS := $(shell llvm-config --ldflags --libs core)

# --- layout -----------------------------------------------------------------
SRC_DIR  := src
TEST_DIR := test-cases
OBJ_DIR  := obj
BIN_DIR  := bin

# This line lets the single pattern rule $(OBJ_DIR)/%.o: %.c pick up .c files 
# from either directory, so you no longer need a separate rule for %-main.o.
VPATH    := $(SRC_DIR) $(TEST_DIR)          # <-- one search path for all .c files

# --- core sources -----------------------------------------------------------
SRC_FILES  := $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJ_FILES  := $(addprefix $(OBJ_DIR)/,$(SRC_FILES:.c=.o))
CORE_OBJS  := $(filter-out $(OBJ_DIR)/main.o,$(OBJ_FILES))

# --- test discovery ---------------------------------------------------------
TEST_DRIVERS := $(notdir $(wildcard $(TEST_DIR)/*-main.c))
TEST_NAMES   := $(TEST_DRIVERS:-main.c=)          # foo-main.c → foo
TEST_BINS    := $(addprefix $(BIN_DIR)/test-,$(TEST_NAMES))

# --- pattern rules ----------------------------------------------------------
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/mycompiler: $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BIN_DIR)/test-%: $(CORE_OBJS) $(OBJ_DIR)/%-main.o | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "✓ built $(@F)"

# --- convenience targets ----------------------------------------------------
.PHONY: all tests test-% clean
all: $(BIN_DIR)/mycompiler

tests: $(TEST_BINS)                 # build every test binary

test-%: | $(BIN_DIR)/test-%         # alias: 'make test-lexer'
	@echo "✓ $< is built"

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# --- automatic directory creation ------------------------------------------
$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@