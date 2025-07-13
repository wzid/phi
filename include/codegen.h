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
} CodeGen;

CodeGen *init_codegen(const char *module_name);
void cleanup_codegen(CodeGen *this);
LLVMValueRef codegen_program(CodeGen *this, Program *program);
LLVMValueRef codegen_expr(CodeGen *this, Expr *expr);
void codegen_stmt(CodeGen *this, Stmt *stmt);

// Utility functions
void dump_ir(CodeGen *this);
void write_bitcode(CodeGen *this, const char *filename);
int run_jit(CodeGen *this);

#endif
