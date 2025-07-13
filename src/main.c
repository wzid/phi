#include <stdio.h>
#include <stdlib.h>

#include "codegen.h"
#include "lexer.h"
#include "memory.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Open the file
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate buffer
    char *buffer = (char *)s_malloc(file_size + 1);
    if (!buffer) {
        perror("Error allocating memory");
        fclose(file);
        return 1;
    }

    // read the file into the buffer
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';  // Null-terminate the string
    fclose(file);

    Lexer lexer = {
        .start_tok = buffer,
        .cur_tok = buffer,
        .line_start = buffer,
    };

    // Call the lex function (to be implemented)
    int result = lex(&lexer);

    if (result == 0) {
        printf("✅ Lexing successful\n");

        // Initialize parser
        Parser parser = init_parser(&lexer);

        // Parse the program
        Program *prog = parse(&parser);

        if (prog) {
            printf("✅ Parsing successful\n");

            // Initialize code generator
            CodeGen *codegen = init_codegen("phi_module");

            // Generate LLVM IR
            LLVMValueRef main_func = codegen_program(codegen, prog);

            if (main_func) {
                printf("✅ Code generation successful\n");

                // Print the generated LLVM IR
                printf("\nGenerated LLVM IR:\n");
                dump_ir(codegen);

                // Optionally run the code using JIT
                printf("\nExecuting code...\n");
                int exit_code = run_jit(codegen);
                printf("Program exited with code: %d\n", exit_code);
            } else {
                printf("Code generation failed\n");
            }

            // Clean up
            cleanup_codegen(codegen);
            free_program(prog);
        } else {
            printf("Parsing failed\n");
        }
    } else {
        printf("Lexing failed\n");
    }

    // Free the buffer
    s_free(buffer);
    free_lexer(&lexer);
    return 0;
}