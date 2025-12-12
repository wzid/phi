#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>

#include "ast.h"

typedef struct {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMExecutionEngineRef engine;
    // Symbol Table
    char **var_names;
    LLVMValueRef *var_allocas;
    int var_count;
    int var_capacity;
} CodeGen;

CodeGen *init_codegen(const char *module_name);
void cleanup_codegen(CodeGen *this);
LLVMValueRef codegen_program(CodeGen *this, Program *program);
LLVMValueRef codegen_expr(CodeGen *this, Expr *expr);
int codegen_stmt(CodeGen *this, Stmt *stmt);

// Utility functions
void dump_ir(CodeGen *this);
void write_ir(CodeGen *this, const char *filename);
int run_jit(CodeGen *this);
void setup_stdlib(CodeGen *this);
LLVMValueRef handle_stdlib_call(CodeGen *this, const char *func_name, Expr **args);
LLVMTypeRef get_type(TokenData type, LLVMContextRef context);

#endif
