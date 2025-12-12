#include "std_lib.h"

void setup_printf(CodeGen* this) {
    // Declare printf function
    LLVMTypeRef printf_arg_types[] = { LLVMPointerType(LLVMInt8TypeInContext(this->context), 0) };
    LLVMTypeRef printf_type = LLVMFunctionType(
        LLVMInt32TypeInContext(this->context), printf_arg_types, 1, 1 // 1 = isVarArg
    );
    LLVMValueRef printf_func = LLVMAddFunction(this->module, "printf", printf_type);
    LLVMSetLinkage(printf_func, LLVMExternalLinkage);
}

LLVMValueRef call_printf(CodeGen* this, Expr **args) {
    // TODO: include the format as one of the arguments
    // This should be done when we add support for string literals
    LLVMValueRef fmt = LLVMBuildGlobalStringPtr(this->builder, "%d\n", "fmt");
    LLVMValueRef llvm_args[] = { fmt, codegen_expr(this, args[0]) };
    LLVMValueRef printf_func = LLVMGetNamedFunction(this->module, "printf");
    return LLVMBuildCall2(this->builder, LLVMGlobalGetValueType(printf_func), printf_func, llvm_args, 2, "");
}

/* (Need to implement Doubles and casting maybe)
void setup_pow(CodeGen* this) {
    // Declare pow function
    LLVMTypeRef pow_arg_types[] = { LLVMDoubleTypeInContext(this->context), LLVMDoubleTypeInContext(this->context) };
    LLVMTypeRef pow_type = LLVMFunctionType(
        LLVMDoubleTypeInContext(this->context), pow_arg_types, 2, 0 // 0 = not vararg
    );
    LLVMValueRef pow_func = LLVMAddFunction(this->module, "pow", pow_type);
    LLVMSetLinkage(pow_func, LLVMExternalLinkage);
}

LLVMValueRef call_pow(CodeGen* this, Expr **args) {
    LLVMValueRef arg0 = codegen_expr(this, args[0]);
    LLVMValueRef arg1 = codegen_expr(this, args[1]);
    LLVMTypeRef double_type = LLVMDoubleTypeInContext(this->context);
  // Cast to double if needed
    if (LLVMGetTypeKind(LLVMTypeOf(arg0)) == LLVMIntegerTypeKind) {
        arg0 = LLVMBuildSIToFP(this->builder, arg0, double_type, "casttmp0");
    }
    if (LLVMGetTypeKind(LLVMTypeOf(arg1)) == LLVMIntegerTypeKind) {
        arg1 = LLVMBuildSIToFP(this->builder, arg1, double_type, "casttmp1");
    }

    LLVMValueRef llvm_args[] = { arg0, arg1 };
    LLVMValueRef pow_func = LLVMGetNamedFunction(this->module, "pow");
    return LLVMBuildCall2(this->builder, LLVMGlobalGetValueType(pow_func), pow_func, llvm_args, 2, "");
}
*/