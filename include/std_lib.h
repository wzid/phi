#ifndef STDLIB_H
#define STDLIB_H

#include "codegen.h"

static const char* stdlib_functions[] = {
    "printf",
    // "pow",
    NULL
};

void setup_printf(CodeGen* this);
LLVMValueRef call_printf(CodeGen* this, Expr **args);

/*
void setup_pow(CodeGen* this);
LLVMValueRef call_pow(CodeGen* this, Expr **args);
*/


#endif