# phi φ
An LLVM enabled compiler written in C

The name is inspired from [Euler's totient function](https://en.wikipedia.org/wiki/Euler%27s_totient_function) which is also called Euler's phi function.

Watch my devlogs on YouTube [here](https://www.youtube.com/watch?v=QPHYcFLAWoo&list=PLEgtx_e7NiZeAXGu8U04pLPKDU5zTNG4M&index=3)!


## syntax
```
func get_name(): string => "phi";

// it would be cool if I could remove the : int and just infer the return type
func add_five(a: int): int => a + 5;

func main() {
    string name = get_name();
    int result = add_five(5);
    print(name);
    print(result);
}
```

## Prerequisites
You will need llvm installed along with coreutils on MacOS.
I forget how to install llvm correctly, it may just be `brew install llvm`
CoreUtils is `brew install coreutils`

## Build Commands

The project uses a Makefile to build and test the compiler. Here are the available commands:

### Building the Compiler
- `make` or `make all` - Builds the main compiler executable (`bin/mycompiler`)
- `make bin/mycompiler` - Explicitly builds the main compiler

### Running the Compiler on a Single File
To compile a single `.phi` file with the main compiler:
```bash
make                    # First build the compiler
./bin/mycompiler input.phi
```

### Testing Commands
- `make test-all` - Builds all test executables (lexer, parser, etc.)
- `make test-lexer` - Builds the lexer test executable and runs all lexer tests
- `make test-parser` - Builds the parser test executable and runs all parser tests
- `make bin/test-lexer` - Only builds the lexer test executable (without running tests)
- `make bin/test-parser` - Only builds the parser test executable (without running tests)

### Running Tests on Individual Files
To test the lexer or parser on a specific file:
```bash
make test-lexer         # Build and run all lexer tests
./bin/test-lexer input.phi  # Run lexer on a single file
```

### Cleaning
- `make clean` - Removes all compiled objects, binaries, and test output files
- `make clean-tests` - Only removes test output files (`.out` and `.diff` files)

### Test Structure
The test system works by:
1. Running your compiler component on `.phi` files in `test-cases/`
2. Comparing the output with expected results in `.ans` files
3. Showing differences in `.diff` files if tests fail

Test files are organized in subdirectories like `test-cases/lexer/` and `test-cases/parser/`.
