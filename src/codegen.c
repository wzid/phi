#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

// helper: simple dynamic symbol table
static void ensure_var_capacity(CodeGen *this) {
    if (this->var_capacity == 0) {
        this->var_capacity = 8;
        this->var_names = s_malloc(sizeof(char*) * this->var_capacity);
        this->var_allocas = s_malloc(sizeof(LLVMValueRef) * this->var_capacity);
    } else if (this->var_count >= this->var_capacity) {
        int newcap = this->var_capacity * 2;
        this->var_names = s_realloc(this->var_names, sizeof(char*) * newcap);
        this->var_allocas = s_realloc(this->var_allocas, sizeof(LLVMValueRef) * newcap);
        this->var_capacity = newcap;
    }
}

static void codegen_set_var(CodeGen *this, const char *name, LLVMValueRef alloc) {
    ensure_var_capacity(this);
    this->var_names[this->var_count] = strdup(name);
    this->var_allocas[this->var_count] = alloc;
    this->var_count++;
}

static LLVMValueRef codegen_get_var(CodeGen *this, const char *name) {
    for (int i = 0; i < this->var_count; ++i) {
        if (!strcmp(this->var_names[i], name)) 
            return this->var_allocas[i];
    }
    return NULL;
}

CodeGen *init_codegen(const char *module_name) {
    CodeGen *codegen = s_malloc(sizeof(CodeGen));

    // Initialize symbol table fields
    codegen->var_names = NULL;
    codegen->var_allocas = NULL;
    codegen->var_count = 0;
    codegen->var_capacity = 0;

    // Initialize LLVM
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    // Create context, module, and builder
    codegen->context = LLVMContextCreate();
    codegen->module = LLVMModuleCreateWithNameInContext(module_name, codegen->context);
    codegen->builder = LLVMCreateBuilderInContext(codegen->context);

    // Create execution engine
    char *error = NULL;
    if (LLVMCreateExecutionEngineForModule(&codegen->engine, codegen->module, &error) != 0) {
        fprintf(stderr, "Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    return codegen;
}

void cleanup_codegen(CodeGen *this) {
    if (this) {
        if (this->var_names) {
            for (int i = 0; i < this->var_count; ++i) {
                s_free(this->var_names[i]);
            }
            s_free(this->var_names);
        }
        if (this->var_allocas)
            s_free(this->var_allocas);

        LLVMDisposeBuilder(this->builder);
        LLVMDisposeExecutionEngine(this->engine);
        LLVMContextDispose(this->context);
        s_free(this);
    }
}

LLVMValueRef codegen_program(CodeGen *this, Program *program) {
    // Create a main function
    LLVMTypeRef int32_type = LLVMInt32TypeInContext(this->context);
    LLVMTypeRef main_func_type = LLVMFunctionType(int32_type, NULL, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(this->module, "main", main_func_type);

    // Create basic block
    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlockInContext(this->context, main_func, "entry");
    LLVMPositionBuilderAtEnd(this->builder, entry_block);

    // Generate code for all statements
    for (int i = 0; i < program->stmt_count; i++) {
        int result = codegen_stmt(this, program->statements[i]);
        // It is equal to 1 if a return statement was generated
        if (result) {
            return main_func;
        }
    }

    // If no explicit return, return 0
    LLVMValueRef zero = LLVMConstInt(int32_type, 0, 0);
    LLVMBuildRet(this->builder, zero);

    return main_func;
}

LLVMValueRef codegen_expr(CodeGen *this, Expr *expr) {
    switch (expr->type) {
        case EXPR_IDENTIFIER: {
            LLVMValueRef var_alloca = codegen_get_var(this, expr->identifier.tok.val);
            if (!var_alloca) {
                fprintf(stderr, "Undefined variable: %s\n", expr->identifier.tok.val);
                return NULL;
            }
            LLVMTypeRef elemType = LLVMGetAllocatedType(var_alloca);

            return LLVMBuildLoad2(this->builder, elemType, var_alloca, expr->identifier.tok.val);
        }
        case EXPR_LITERAL_INT: {
            // Parse integer literal
            int value = atoi(expr->int_literal.value);
            LLVMTypeRef int32_type = LLVMInt32TypeInContext(this->context);
            return LLVMConstInt(int32_type, value, 0);
        }

        case EXPR_BINARY: {
            LLVMValueRef left = codegen_expr(this, expr->binary.left);
            LLVMValueRef right = codegen_expr(this, expr->binary.right);

            switch (expr->binary.op_token.type) {
                case tok_plus:
                    return LLVMBuildAdd(this->builder, left, right, "addtmp");
                case tok_minus:
                    return LLVMBuildSub(this->builder, left, right, "subtmp");
                case tok_star:
                    return LLVMBuildMul(this->builder, left, right, "multmp");
                case tok_slash:
                    return LLVMBuildSDiv(this->builder, left, right, "divtmp");
                case tok_mod:
                    return LLVMBuildSRem(this->builder, left, right, "modtmp");
                case tok_equality:
                    return LLVMBuildICmp(this->builder, LLVMIntEQ, left, right, "eqtmp");
                case tok_inequality:
                    return LLVMBuildICmp(this->builder, LLVMIntNE, left, right, "netmp");
                case tok_lessthan:
                    return LLVMBuildICmp(this->builder, LLVMIntSLT, left, right, "lttmp");
                case tok_greaterthan:
                    return LLVMBuildICmp(this->builder, LLVMIntSGT, left, right, "gttmp");
                default:
                    fprintf(stderr, "Unknown binary operator\n");
                    return NULL;
            }
        }

        case EXPR_UNARY: {
            LLVMValueRef operand = codegen_expr(this, expr->unary.right);

            switch (expr->unary.op_token.type) {
                case tok_minus: {
                    LLVMTypeRef int32_type = LLVMInt32TypeInContext(this->context);
                    LLVMValueRef zero = LLVMConstInt(int32_type, 0, 0);
                    return LLVMBuildSub(this->builder, zero, operand, "negtmp");
                }
                case tok_not: {
                    LLVMTypeRef bool_type = LLVMInt1TypeInContext(this->context);
                    LLVMValueRef true_val = LLVMConstInt(bool_type, 1, 0);
                    return LLVMBuildXor(this->builder, operand, true_val, "nottmp");
                }
                default:
                    fprintf(stderr, "Unknown unary operator\n");
                    return NULL;
            }
        }

        case EXPR_LITERAL_BOOL: {
            LLVMTypeRef bool_type = LLVMInt1TypeInContext(this->context);
            return LLVMConstInt(bool_type, expr->bool_literal.value, 0);
        }

        case EXPR_LITERAL_STRING:
            // String literals would need more complex handling
            fprintf(stderr, "String literals not yet implemented\n");
            return NULL;

        case EXPR_CALL:
            // Function calls would need more complex handling
            fprintf(stderr, "Function calls not yet implemented\n");
            return NULL;

        default:
            fprintf(stderr, "Unknown expression type\n");
            return NULL;
    }
}

int codegen_stmt(CodeGen *this, Stmt *stmt) {
    switch (stmt->type) {
        case STMT_EXPR: {
            // Generate code for expression but don't use the result
            codegen_expr(this, stmt->expression_stmt.value);
            break;
        }

        case STMT_RETURN: {
            // This should be handled in codegen_program
            LLVMValueRef return_val = codegen_expr(this, stmt->return_stmt.value);
            LLVMBuildRet(this->builder, return_val);
            return 1;
        }

        case STMT_BLOCK: {
            // Generate code for all statements in the block
            for (int i = 0; i < stmt->block_stmt.stmt_count; i++) {
                codegen_stmt(this, stmt->block_stmt.statements[i]);
            }
            break;
        }
        case STMT_VAR_DECL: {

            // Evaluate initializer (if any)
            LLVMValueRef init_val = NULL;
            if (stmt->var_decl.value) {
                init_val = codegen_expr(this, stmt->var_decl.value);
            }

            // If no initializer, use a typed zero/null
            if (!init_val) {
                // here we assume var_decl has type info if needed; fallback to int32 zero
                LLVMTypeRef t = LLVMInt32TypeInContext(this->context);
                init_val = LLVMConstNull(t);
            }

            // Create the alloca in the entry block of the current function
            LLVMBasicBlockRef cur_bb = LLVMGetInsertBlock(this->builder);
            LLVMValueRef cur_fn = LLVMGetBasicBlockParent(cur_bb);
            LLVMBasicBlockRef entry_bb = LLVMGetFirstBasicBlock(cur_fn);

            // temporary builder placed at start of entry block
            LLVMBuilderRef tmp_builder = LLVMCreateBuilderInContext(this->context);
            LLVMValueRef first_instr = LLVMGetFirstInstruction(entry_bb);
            if (first_instr) {
                LLVMPositionBuilderBefore(tmp_builder, first_instr);
            } else {
                LLVMPositionBuilderAtEnd(tmp_builder, entry_bb);
            }

            LLVMTypeRef var_type = LLVMTypeOf(init_val);
            LLVMValueRef alloca = LLVMBuildAlloca(tmp_builder, var_type, stmt->var_decl.tok_identifier.val);

            LLVMDisposeBuilder(tmp_builder);

            // store initializer into the alloca using current builder (current insertion point)
            LLVMBuildStore(this->builder, init_val, alloca);

            // register in symbol table
            codegen_set_var(this, stmt->var_decl.tok_identifier.val, alloca);

            break;
        }

        default:
            fprintf(stderr, "Unknown statement type\n");
            break;
    }

    return 0;
}

void dump_ir(CodeGen *this) {
    char *ir = LLVMPrintModuleToString(this->module);
    printf("%s\n", ir);
    LLVMDisposeMessage(ir);
}

void write_bitcode(CodeGen *this, const char *filename) {
    if (LLVMWriteBitcodeToFile(this->module, filename) != 0) {
        fprintf(stderr, "Error writing bitcode to file %s\n", filename);
    }
}

int run_jit(CodeGen *this) {
    // Verify the module
    char *error = NULL;
    if (LLVMVerifyModule(this->module, LLVMAbortProcessAction, &error) != 0) {
        fprintf(stderr, "Module verification failed: %s\n", error);
        LLVMDisposeMessage(error);
        return -1;
    }

    // Find and run the main function
    LLVMValueRef main_func = LLVMGetNamedFunction(this->module, "main");
    if (!main_func) {
        fprintf(stderr, "Main function not found\n");
        return -1;
    }

    // Execute the function
    LLVMGenericValueRef result = LLVMRunFunction(this->engine, main_func, 0, NULL);
    int return_value = LLVMGenericValueToInt(result, 0);

    LLVMDisposeGenericValue(result);
    return return_value;
}
