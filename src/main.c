#include <stdio.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/BitWriter.h>

int main() {
    // Initialize LLVM
    LLVMContextRef context = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext("my_module", context);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    
    // Set target triple for M1 Mac
    LLVMSetTarget(module, "arm64-apple-macosx15.0.0");
    
    // Get reference to the printf function
    LLVMTypeRef params[] = { LLVMPointerType(LLVMInt8TypeInContext(context), 0) };
    LLVMTypeRef printfType = LLVMFunctionType(LLVMInt32TypeInContext(context), params, 1, 1);
    LLVMValueRef printfFunc = LLVMAddFunction(module, "printf", printfType);
    
    // Create a function type for main (returns int)
    LLVMTypeRef mainFuncType = LLVMFunctionType(LLVMInt32TypeInContext(context), NULL, 0, 0);
    
    // Create the main function
    LLVMValueRef mainFunc = LLVMAddFunction(module, "main", mainFuncType);
    
    // Create a basic block
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context, mainFunc, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    
    // Create a global string for the format
    LLVMValueRef formatStr = LLVMBuildGlobalStringPtr(builder, "%d\n", "format");
    
    // Create arguments for printf (format string and the number 3)
    LLVMValueRef args[2];
    args[0] = formatStr;
    args[1] = LLVMConstInt(LLVMInt32TypeInContext(context), 3, 0);
    
    // Call printf with the format string and the number 3
    LLVMBuildCall2(builder, printfType, printfFunc, args, 2, "");
    
    // Return 0 from main
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32TypeInContext(context), 0, 0));
    
    // Verify the module
    char *error = NULL;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    
    // Print the generated LLVM IR
    LLVMDumpModule(module);
    
    // Write bitcode to a file
    if (LLVMWriteBitcodeToFile(module, "output.bc") != 0) {
        fprintf(stderr, "Error writing bitcode to file\n");
    }
    
    // Clean up
    LLVMDisposeBuilder(builder);
    LLVMDisposeModule(module);
    LLVMContextDispose(context);
    
    return 0;
}