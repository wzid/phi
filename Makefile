# This line lets the single pattern rule obj/%.o: %.c pick up .c files 
# from either directory, so you no longer need a separate rule for %-main.o.
VPATH    := src test-cases          # <-- one search path for all .c files

SRC_FILES  := $(notdir $(wildcard src/*.c))
OBJ_FILES  := $(addprefix obj/,$(SRC_FILES:.c=.o))
CORE_OBJS  := $(filter-out obj/main.o,$(OBJ_FILES))

TEST_DRIVERS := $(notdir $(wildcard test-cases/*-main.c))
TEST_NAMES   := $(TEST_DRIVERS:-main.c=)          # foo-main.c → foo
TEST_BINS    := $(addprefix test-,$(TEST_NAMES))

BOLD := \033[1m
RESET := \033[0m
GREEN := \033[0;32m
BLUE := \033[0;34m

help: ## Shows all commands
	@printf "%b\n" "$(BOLD)All Makefile commands:$(RESET)"
	@printf " %b - builds the main compiler binary\n" "$(GREEN)$(BOLD)make all$(RESET)"
	@printf " %b - builds all test binaries\n" "$(GREEN)$(BOLD)make test-all$(RESET)"
	@printf " %b - builds and runs tests for <name> (e.g., lexer, parser)\n" "$(GREEN)$(BOLD)make test-<name>$(RESET)"
	@printf " %b - removes all build files\n" "$(GREEN)$(BOLD)make clean$(RESET)"
	@printf " %b - removes all test output files\n" "$(GREEN)$(BOLD)make clean-tests$(RESET)"

obj/%.o: %.c | obj
	clang -Wall -Wextra $(shell llvm-config --cflags) -I./include -c $< -o $@

# $(OBJ_FILES) calls the rule above
bin/mycompiler: $(OBJ_FILES) | bin
	clang $^ -o $@ $(shell llvm-config --ldflags --libs core)


bin/test-%: $(CORE_OBJS) obj/%-main.o | bin
	clang $^ -o $@ $(shell llvm-config --ldflags --libs core)
	@echo "✓ built $(@F)"

all: bin/mycompiler

test-all: $(TEST_BINS)

test-%: bin/test-%         # alias: 'make test-lexer'
	@echo "✓ $< is built"
	@echo "Running tests..."
	./test.sh $*

# loop through all test folders and remove .out files
clean-tests:
	@echo "Cleaning test files"
	@for dir in $(TEST_NAMES); do \
		rm -f test-cases/$${dir}/*.out test-cases/$${dir}/*.diff; \
	done

clean: clean-tests
	rm -rf obj bin

# --- automatic directory creation ------------------------------------------
bin obj:
	@echo "Creating directory $@"
	mkdir -p $@

.PHONY: all test-all test-% clean help
.PRECIOUS: obj/%.o