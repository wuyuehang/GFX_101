#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
using namespace llvm;

int main() {
    LLVMContext TheContext;
    Module *TheModule = new Module("HelloLLVM", TheContext);

    IRBuilder<> TheBuilder(TheContext);
    {
        FunctionType *funcType = FunctionType::get(TheBuilder.getInt32Ty(), false);
        Function *fooFunc = Function::Create(funcType, Function::ExternalLinkage, "foo", TheModule);
        verifyFunction(*fooFunc);
    }

    TheModule->dump();
    delete TheModule;
    return 0;
}