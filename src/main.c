#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <string.h>

#include "codegen.h"
#include "lexer.h"
#include "memory.h"
#include "parser.h"

void optimize_module(CodeGen *this) {
    // Create pass builder options
    LLVMPassBuilderOptionsRef opts = LLVMCreatePassBuilderOptions();

    // Set debug options if you want (optional)
    LLVMPassBuilderOptionsSetVerifyEach(opts, 0);
    LLVMPassBuilderOptionsSetDebugLogging(opts, 0);

    // Use the "default<O2>" pipeline, just like: opt -passes="default<O2>"
    const char *pipeline = "default<O2>";

    // If you don't have a target machine, pass NULL
    LLVMTargetMachineRef tm = NULL;

    LLVMErrorRef err = LLVMRunPasses(
        this->module,
        pipeline,
        tm,
        opts
    );

    if (err) {
        char *msg = LLVMGetErrorMessage(err);
        fprintf(stderr, "LLVM optimization error: %s\n", msg);
        LLVMDisposeErrorMessage(msg);
    }

    LLVMDisposePassBuilderOptions(opts);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <source> [-o <output>] [--optimize]\n", argv[0]);
        return 1;
    }

    const char *output_file = NULL;
    int emit_binary = 0;
    int optimize = 0;
    int print_ir = 0;

    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc) {
            output_file = argv[++i];
            emit_binary = 1;
        } else if (!strcmp(argv[i], "--optimize") || !strcmp(argv[i], "-O")) {
            optimize = 1;
        } else if (!strcmp(argv[i], "--print-ir") || !strcmp(argv[i], "-p")) {
            print_ir = 1;
        }
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

    if (result == 1) {
        printf("Lexing failed\n");
        s_free(buffer);
        free_lexer(&lexer);
        return 1;
    }

    printf("✅ Lexing successful\n");

    // Initialize parser
    Parser parser = init_parser(&lexer);

    // Parse the program
    Program *prog = parse(&parser);

    if (!prog) {
        printf("Parsing failed\n");
        s_free(buffer);
        free_lexer(&lexer);
        return 1;
    }
    printf("✅ Parsing successful\n");

    // Initialize code generator
    CodeGen *codegen = init_codegen("phi_module");

    // Generate LLVM IR
    LLVMValueRef main_func = codegen_program(codegen, prog);

    if (!main_func) {
        printf("Code generation failed\n");
        cleanup_codegen(codegen);
        free_program(prog);
        s_free(buffer);
        free_lexer(&lexer);
        return 1;
    }
    printf("✅ Code generation successful\n");

    // Print the generated LLVM IR
    if (print_ir) {
        printf("\nGenerated LLVM IR:\n");
        dump_ir(codegen);
    }

    if (optimize) {
        // Optimize the module
        optimize_module(codegen);
        if (print_ir) {
            printf("\nOptimized LLVM IR:\n");
            dump_ir(codegen);
        }
    }


    // Optionally run the code using JIT
    if (emit_binary) {
        write_ir(codegen, "out.ll");
        printf("Wrote LLVM IR to out.ll\n");

        // Call clang to compile the bitcode to a binary
        char command[512];
        snprintf(command, sizeof(command), "clang -Wno-override-module out.ll -o %s", output_file);
        int ret = system(command);
        if (ret != 0) {
            printf("Failed to compile bitcode to binary\n");
        } else {
            printf("✅ Compiled to binary: %s\n", output_file);
        }

    } else {
        printf("\nExecuting code...\n");
        int exit_code = run_jit(codegen);
        printf("Program exited with code: %d\n", exit_code);
    }

    // Clean up
    cleanup_codegen(codegen);
    free_program(prog);
    // Free the buffer
    s_free(buffer);
    free_lexer(&lexer);
    return 0;
}