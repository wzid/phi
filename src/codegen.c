#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "std_lib.h"

// helper: simple dynamic symbol table
static void ensure_var_capacity(CodeGen* this) {
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

// Set variable in symbol table
static void codegen_set_var(CodeGen* this, const char* name, LLVMValueRef alloc) {
    ensure_var_capacity(this);
    this->var_names[this->var_count] = strdup(name);
    this->var_allocas[this->var_count] = alloc;
    this->var_count++;
}

// Get variable from symbol table
static LLVMValueRef codegen_get_var(CodeGen* this, const char* name) {
    for (int i = 0; i < this->var_count; ++i) {
        if (!strcmp(this->var_names[i], name)) return this->var_allocas[i];
    }
    return NULL;
}

CodeGen* init_codegen(const char* module_name) {
    CodeGen* codegen = s_malloc(sizeof(CodeGen));

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
    char* error = NULL;
    if (LLVMCreateExecutionEngineForModule(&codegen->engine, codegen->module, &error) != 0) {
        fprintf(stderr, "Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        exit(1);
    }

    return codegen;
}

void cleanup_codegen(CodeGen* this) {
    if (this) {
        if (this->var_names) {
            for (int i = 0; i < this->var_count; ++i) {
                s_free(this->var_names[i]);
            }
            s_free(this->var_names);
        }
        if (this->var_allocas) s_free(this->var_allocas);

        LLVMDisposeBuilder(this->builder);
        LLVMDisposeExecutionEngine(this->engine);
        LLVMContextDispose(this->context);
        s_free(this);
    }
}

// this function was abstracted away from codegen_program to get function type
// it was getting too big otherwise
LLVMTypeRef get_function_type(CodeGen* this, FuncDeclStmt* func_decl) {
    TokenData return_type = func_decl->tok_return_type;
    LLVMTypeRef ret_type = LLVMVoidTypeInContext(this->context);

    // TODO: handle other return types (how to handle user-defined types?)
    // if the return type is 'int' or if the function is 'main', return int
    if ((return_type.val && strcmp(return_type.val, "int") == 0) || strcmp(func_decl->tok_identifier.val, "main") == 0) {
        ret_type = LLVMInt32TypeInContext(this->context);
    } else if (return_type.val && strcmp(return_type.val, "bool") == 0) {
        ret_type = LLVMInt1TypeInContext(this->context);
    } else if (return_type.val && strcmp(return_type.val, "string") == 0) {
        ret_type = LLVMPointerType(LLVMInt8TypeInContext(this->context), 0);
    }

    if (func_decl->parameter_count > 0) {
        LLVMTypeRef* param_types = s_malloc(sizeof(LLVMTypeRef) * func_decl->parameter_count);
        for (int j = 0; j < func_decl->parameter_count; j++) {
            // For simplicity, we only handle 'int' parameter types for now
            TokenData param_type = func_decl->parameter_types[j];
            // TODO: handle other parameter types
            if (param_type.type == tok_type && strcmp(param_type.val, "int") == 0) {
                param_types[j] = LLVMInt32TypeInContext(this->context);
            } else {
                param_types[j] = LLVMInt32TypeInContext(this->context);  // default to int
            }
        }
        LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, func_decl->parameter_count, 0);
        s_free(param_types);
        return func_type;
    } else {
        return LLVMFunctionType(ret_type, NULL, 0, 0);
    }
}

LLVMValueRef codegen_program(CodeGen* this, Program* program) {
    // 1) Create prototypes for all functions so calls work correctly
    // and create global variables
    for (int i = 0; i < program->stmt_count; i++) {
        Stmt* stmt = program->statements[i];
        switch (stmt->type) {
            case STMT_FUNC_DECL: {
                FuncDeclStmt* func_decl = &stmt->func_decl;
                LLVMTypeRef func_type = get_function_type(this, func_decl);

                LLVMAddFunction(this->module, stmt->func_decl.tok_identifier.val, func_type);
                break;
            }
            case STMT_GLOBAL_VAR_DECL: {
                // Handle global variable declaration
                GlobalVarDeclStmt *global_var_decl = &stmt->global_var_decl;

                // For simplicity, we only handle 'int' type globals for now
                LLVMTypeRef var_type = get_type(global_var_decl->type, this->context);
                LLVMValueRef global_var = LLVMAddGlobal(this->module, var_type, global_var_decl->tok_identifier.val);
                LLVMValueRef init_val = codegen_expr(this, global_var_decl->value);
                LLVMSetInitializer(global_var, init_val);
                // No need to add to symbol table since it's global
                break;
            }
            default:
                break;
        }
    }

    // Add functions from standard library
    setup_stdlib(this);

    // 2) Generate code for all statements
    for (int i = 0; i < program->stmt_count; i++) {
        Stmt* stmt = program->statements[i];
        codegen_stmt(this, stmt);
    }

    return LLVMGetNamedFunction(this->module, "main");
}

LLVMValueRef codegen_expr(CodeGen* this, Expr* expr) {
    switch (expr->type) {
        case EXPR_IDENTIFIER: {
            // Load variable value for use in an expression

            // 1) See if the variable is in the symbol table
            LLVMValueRef var_alloca = codegen_get_var(this, expr->identifier.tok.val);
            if (var_alloca) {
                LLVMTypeRef elemType = LLVMGetAllocatedType(var_alloca);
                return LLVMBuildLoad2(this->builder, elemType, var_alloca, expr->identifier.tok.val);
            }

            // 2) If not found, it might be a global variable
            LLVMValueRef global_var = LLVMGetNamedGlobal(this->module, expr->identifier.tok.val);
            if (global_var) {
                LLVMTypeRef var_type = LLVMGlobalGetValueType(global_var);
                return LLVMBuildLoad2(this->builder, var_type, global_var, expr->identifier.tok.val);
            }
            
            fprintf(stderr, "Undefined variable: %s\n", expr->identifier.tok.val);
            return NULL;
        }
        case EXPR_LITERAL_INT: {
            // Parse integer literal
            int value = atoi(expr->int_literal.value);
            LLVMTypeRef int32_type = LLVMInt32TypeInContext(this->context);
            return LLVMConstInt(int32_type, value, 0);
        }
        case EXPR_BINARY: {
            // Generate code for left and right expressions
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
        case EXPR_INCREMENT: {
            LLVMValueRef var_alloca = codegen_get_var(this, expr->increment.identifier.val);
            LLVMValueRef global_var = NULL;
            if (!var_alloca) {
                global_var = LLVMGetNamedGlobal(this->module, expr->increment.identifier.val);
            }
            if (!var_alloca && !global_var) {
                fprintf(stderr, "Undefined variable in increment: %s\n", expr->increment.identifier.val);
                return NULL;
            }

            LLVMTypeRef elemType = var_alloca ? LLVMGetAllocatedType(var_alloca) : LLVMGlobalGetValueType(global_var);
            LLVMValueRef ptr = var_alloca ? var_alloca : global_var;
            LLVMValueRef current_val = LLVMBuildLoad2(this->builder, elemType, ptr, "loadtmp");
            LLVMValueRef one = LLVMConstInt(elemType, 1, 0);

            LLVMValueRef new_val = NULL;
            if (expr->increment.op_token.type == tok_increment) {
                new_val = LLVMBuildAdd(this->builder, current_val, one, "inctmp");
            } else if (expr->increment.op_token.type == tok_decrement) {
                new_val = LLVMBuildSub(this->builder, current_val, one, "dectmp");
            } else {
                fprintf(stderr, "Unknown increment operator\n");
                return NULL;
            }

            LLVMBuildStore(this->builder, new_val, ptr);

            // Handle prefix vs postfix
            if (expr->increment.is_prefix) {
                // Prefix: return new value
                return new_val;
            } else {
                // Postfix: return original value
                return current_val;
            }
        }

        case EXPR_UNARY: {
            // Generate code for the right expression
            LLVMValueRef operand = codegen_expr(this, expr->unary.right);

            // Handle the unary operator
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
            return LLVMBuildGlobalStringPtr(this->builder, expr->str_literal.value, "strtmp");
        case EXPR_FUNC_CALL: {
            // Check if it's a standard library function
            for (int i = 0; stdlib_functions[i] != NULL; i++) {
                if (strcmp(expr->func_call.tok_function.val, stdlib_functions[i]) == 0) {
                    return handle_stdlib_call(this, expr->func_call.tok_function.val, expr->func_call.args);
                }
            }

            LLVMValueRef callee = LLVMGetNamedFunction(this->module, expr->func_call.tok_function.val);
            if (!callee) {
                fprintf(stderr, "Undefined function: %s\n", expr->func_call.tok_function.val);
                return NULL;
            }

            // Generate code for arguments
            LLVMValueRef* args = NULL;
            if (expr->func_call.arg_count > 0) {
                args = s_malloc(sizeof(LLVMValueRef) * expr->func_call.arg_count);
                for (int i = 0; i < expr->func_call.arg_count; i++) {
                    args[i] = codegen_expr(this, expr->func_call.args[i]);
                }
            }

            // Get the function type (callee is a function pointer/value)
            // https://discourse.llvm.org/t/llvmbuildcall2-function-type/71093/3
            // time spent fixing this: 1 hour
            LLVMTypeRef func_type = LLVMGlobalGetValueType(callee);
            
            // Build the call instruction
            LLVMValueRef call = LLVMBuildCall2(this->builder, func_type, callee, args, expr->func_call.arg_count, "calltmp");
            s_free(args);
            return call;
        }
        default:
            fprintf(stderr, "Unknown expression type\n");
            return NULL;
    }
}

int codegen_stmt(CodeGen* this, Stmt* stmt) {
    switch (stmt->type) {
        case STMT_FUNC_DECL: {
            // To help with function calls, we only handle function declarations at the top level in codegen_program
            // So here we just generate the body of the function
            LLVMValueRef func = LLVMGetNamedFunction(this->module, stmt->func_decl.tok_identifier.val);
            if (!func) {
                fprintf(stderr, "Function not found: %s\n", stmt->func_decl.tok_identifier.val);
                return 0;
            }

            // Create a new basic block for the function body
            LLVMBasicBlockRef entry_block = LLVMAppendBasicBlockInContext(this->context, func, "entry");
            // set builder to end of entry block (it just moves it after the "entry" label so instructions are "under" it)
            LLVMPositionBuilderAtEnd(this->builder, entry_block); 


            // snapshot symbol the table so we can restore it when leaving this function (function-level scope)
            // while keeping the same amount of global variables
            int saved_var_count = this->var_count;

            // Load parameters into allocas and register them in the symbol table
            for (int i = 0; i < stmt->func_decl.parameter_count; i++) {
                LLVMValueRef param = LLVMGetParam(func, i);
                const char* pname = stmt->func_decl.parameter_names[i].val;
                LLVMSetValueName(param, pname);

                // create an alloca in the entry block (use temp builder placed at start)
                LLVMBuilderRef tmp_builder = LLVMCreateBuilderInContext(this->context);
                LLVMValueRef first_instr = LLVMGetFirstInstruction(entry_block);
                if (first_instr)
                    LLVMPositionBuilderBefore(tmp_builder, first_instr);
                else
                    LLVMPositionBuilderAtEnd(tmp_builder, entry_block);

                LLVMTypeRef arg_type = LLVMTypeOf(param);
                LLVMValueRef alloca = LLVMBuildAlloca(tmp_builder, arg_type, pname);
                LLVMDisposeBuilder(tmp_builder);

                // store the incoming param into the alloca and register it
                LLVMBuildStore(this->builder, param, alloca);
                codegen_set_var(this, pname, alloca);
            }

            // Generate code for the function body
            int has_return = codegen_stmt(this, stmt->func_decl.body);

            if (!has_return) {
                // If no return statement was generated, add a default return 0
                LLVMTypeRef ret_type = LLVMGetReturnType(LLVMGetElementType(LLVMTypeOf(func)));
                if (ret_type == LLVMVoidTypeInContext(this->context)) {
                    LLVMBuildRetVoid(this->builder);
                } else {
                    // error handling for non-void functions without return
                    fprintf(stderr, "Error: Non-void function '%s' missing return statement\n",
                            stmt->func_decl.tok_identifier.val);
                    return 0;
                }
            }

            // restore symbol table (pop function-scope parameters)
            for (int i = saved_var_count; i < this->var_count; ++i) {
                s_free(this->var_names[i]);
            }
            this->var_count = saved_var_count;
            return 0;
        }

        case STMT_EXPR: {
            // Generate code for expression but don't use the result
            codegen_expr(this, stmt->expression_stmt.value);
            break;
        }

        case STMT_RETURN: {
            // This should be handled in codegen_program
            LLVMValueRef return_val = codegen_expr(this, stmt->return_stmt.value);
            LLVMBuildRet(this->builder, return_val);
            break;
        }

        case STMT_BLOCK: {
            // Generate code for all statements in the block
            int has_return = 0;
            for (int i = 0; i < stmt->block_stmt.stmt_count; i++) {
                if (stmt->block_stmt.statements[i]->type == STMT_RETURN) {
                    has_return = 1;
                }
                codegen_stmt(this, stmt->block_stmt.statements[i]);
            }
            // return up to the func_decl_stmt to know if a return was encountered
            return has_return;
        }
        case STMT_VAR_DECL: {
            // Evaluate initializer (if any)
            LLVMValueRef init_val = NULL;
            if (stmt->var_decl.value) {
                init_val = codegen_expr(this, stmt->var_decl.value);
            }

            // If no initializer, use a typed zero/null
            if (!init_val) {
                printf("Warning: Variable '%s' declared without initializer, defaulting to zero\n",
                       stmt->var_decl.tok_identifier.val);
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
            // printf("Variable \"%s\" has type: %s\n", stmt->var_decl.tok_identifier.val, LLVMPrintTypeToString(var_type));
            LLVMValueRef alloca = LLVMBuildAlloca(tmp_builder, var_type, stmt->var_decl.tok_identifier.val);
            
            LLVMDisposeBuilder(tmp_builder);
            
            // store initializer into the alloca using current builder (current insertion point)
            LLVMBuildStore(this->builder, init_val, alloca);
            
            // register in symbol table
            codegen_set_var(this, stmt->var_decl.tok_identifier.val, alloca);
            
            
            break;
        }
        case STMT_GLOBAL_VAR_DECL:
            // Global variable declarations are handled at the module level
            // This is a placeholder to avoid errors
            break;
        case STMT_VAR_ASSIGN: {
            // Get the variable's alloca from the symbol table
            LLVMValueRef local_var_alloca = codegen_get_var(this, stmt->var_assign.tok_identifier.val);
            LLVMValueRef global_value = NULL;
            
            // If not found in symbol table, it might be a global variable
            if (!local_var_alloca) {
                global_value = LLVMGetNamedGlobal(this->module, stmt->var_assign.tok_identifier.val);
            }

            if (!local_var_alloca && !global_value) {
                fprintf(stderr, "Undefined variable in assignment: %s\n", stmt->var_assign.tok_identifier.val);
                return 0;
            }

            // Generate code for the value to assign
            LLVMValueRef value = codegen_expr(this, stmt->var_assign.new_value);

            // Simple assignment
            if (stmt->var_assign.modifying_tok.type == tok_equal) {
                if (local_var_alloca) {
                    LLVMBuildStore(this->builder, value, local_var_alloca);
                } else if (global_value) {
                    LLVMBuildStore(this->builder, value, global_value);
                }
                break;
            }
            LLVMTypeRef elemType = NULL;
            LLVMValueRef current_val = NULL;
            
            if (local_var_alloca) {
                elemType = LLVMGetAllocatedType(local_var_alloca);
                current_val = LLVMBuildLoad2(this->builder, elemType, local_var_alloca, "loadtmp");
            } else if (global_value) {
                elemType = LLVMGlobalGetValueType(global_value);
                current_val = LLVMBuildLoad2(this->builder, elemType, global_value, "loadtmp");
            }


            LLVMValueRef value_to_use = local_var_alloca ? local_var_alloca : global_value;            

            // Otherwise store the value into the variable's alloca based on the modifying token
            switch (stmt->var_assign.modifying_tok.type) {
                case tok_plus_equal: {
                    LLVMValueRef new_val = LLVMBuildAdd(this->builder, current_val, value, "addtmp");
                    LLVMBuildStore(this->builder, new_val, value_to_use);
                    break;
                }
                case tok_minus_equal: {
                    LLVMValueRef new_val = LLVMBuildSub(this->builder, current_val, value, "subtmp");
                    LLVMBuildStore(this->builder, new_val, value_to_use);
                    break;
                }
                case tok_star_equal: {
                    LLVMValueRef new_val = LLVMBuildMul(this->builder, current_val, value, "multmp");
                    LLVMBuildStore(this->builder, new_val, value_to_use);
                    break;
                }
                case tok_slash_equal: {
                    LLVMValueRef new_val = LLVMBuildSDiv(this->builder, current_val, value, "divtmp");
                    LLVMBuildStore(this->builder, new_val, value_to_use);
                    break;
                }
                default:
                    fprintf(stderr, "Unknown assignment operator\n");
                    return 0;
            }
            break;
        }
        case STMT_IF: {
            // Generate code for the condition expression
            Expr* cond_expr = stmt->if_stmt.condition;
            LLVMValueRef cond_val = codegen_expr(this, cond_expr);
            if (!cond_val) {
                fprintf(stderr, "Failed to generate code for if condition.\n");
                return 0;
            }

            // Convert condition to bool (i1) if needed
            LLVMTypeRef cond_type = LLVMTypeOf(cond_val);
            if (LLVMGetTypeKind(cond_type) != LLVMIntegerTypeKind || LLVMGetIntTypeWidth(cond_type) != 1) {
                cond_val = LLVMBuildICmp(this->builder, LLVMIntNE, cond_val, LLVMConstInt(cond_type, 0, 0), "ifcond");
            }

            // Get the current function
            LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(this->builder));

            // Create blocks for then, else, and merge
            LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(this->context, func, "then");
            LLVMBasicBlockRef else_bb = NULL;
            LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlockInContext(this->context, func, "ifcont");

            int has_else = (stmt->if_stmt.else_branch != NULL);
            if (has_else) {
                else_bb = LLVMAppendBasicBlockInContext(this->context, func, "else");
            } else {
                else_bb = merge_bb;
            }

            // Build the conditional branch
            LLVMBuildCondBr(this->builder, cond_val, then_bb, else_bb);

            // Emit then block (with scope)
            LLVMPositionBuilderAtEnd(this->builder, then_bb);
            int saved_var_count_then = this->var_count;
            int then_has_return = codegen_stmt(this, stmt->if_stmt.then_branch);
            // Pop variables declared in then block
            for (int i = saved_var_count_then; i < this->var_count; ++i) {
                if (this->var_names && this->var_names[i]) s_free(this->var_names[i]);
            }
            this->var_count = saved_var_count_then;
            if (!then_has_return) {
                LLVMBuildBr(this->builder, merge_bb);
            }

            // Emit else block if present (with scope)
            if (has_else) {
                LLVMPositionBuilderAtEnd(this->builder, else_bb);
                int saved_var_count_else = this->var_count;
                int else_has_return = codegen_stmt(this, stmt->if_stmt.else_branch);
                for (int i = saved_var_count_else; i < this->var_count; ++i) {
                    if (this->var_names && this->var_names[i]) s_free(this->var_names[i]);
                }
                this->var_count = saved_var_count_else;
                if (!else_has_return) {
                    LLVMBuildBr(this->builder, merge_bb);
                }
            }

            // Continue at merge block
            LLVMPositionBuilderAtEnd(this->builder, merge_bb);
            return 0;
        }
        default:
            fprintf(stderr, "Unknown statement type\n");
            break;
    }

    return 0;
}

void dump_ir(CodeGen* this) {
    char* ir = LLVMPrintModuleToString(this->module);
    printf("%s\n", ir);
    LLVMDisposeMessage(ir);
}

void write_ir(CodeGen* this, const char* filename) {
    // Write the LLVM IR (human-readable .ll file)
    char* error = NULL;
    if (LLVMPrintModuleToFile(this->module, filename, &error) != 0) {
        fprintf(stderr, "Error writing LLVM IR to file %s: %s\n", filename, error);
        LLVMDisposeMessage(error);
    }
}

int run_jit(CodeGen* this) {
    // Verify the module
    char* error = NULL;
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

void setup_stdlib(CodeGen* this) {
    setup_printf(this);
    // setup_pow(this);
}

LLVMValueRef handle_stdlib_call(CodeGen* this, const char* func_name, Expr** args) {
    if (strcmp(func_name, "printf") == 0) {
        return call_printf(this, args);
    }/* else if (strcmp(func_name, "pow") == 0) {
        return call_pow(this, args);
    }*/
     else {
        fprintf(stderr, "Unknown standard library function: %s\n", func_name);
        return NULL;
    }
}

LLVMTypeRef get_type(TokenData tok_type, LLVMContextRef context) {
    if (strcmp(tok_type.val, "int") == 0) {
        return LLVMInt32TypeInContext(context);
    } else if (strcmp(tok_type.val, "bool") == 0) {
        return LLVMInt1TypeInContext(context);
    } else if (strcmp(tok_type.val, "string") == 0) {
        return LLVMPointerTypeInContext(context, 0); // char*
    } else {
        fprintf(stderr, "Unknown type: %s\n", tok_type.val);
        return NULL;
    }
}