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

        TheModule->getOrInsertGlobal("g_foo", TheBuilder.getInt32Ty());
        GlobalVariable *gVar = TheModule->getNamedGlobal("g_foo");
        gVar->setLinkage(GlobalValue::CommonLinkage);
        const DataLayout &DL = TheModule->getDataLayout();
        Align Alignment = DL.getValueOrABITypeAlignment(gVar->getAlign(), gVar->getValueType());
        gVar->setAlignment(Alignment);

        BasicBlock *entryBB = BasicBlock::Create(TheContext, "entry", fooFunc);
        TheBuilder.SetInsertPoint(entryBB);
        TheBuilder.CreateRet(TheBuilder.getInt32(0));

        verifyFunction(*fooFunc);
    }

    TheModule->dump();
    delete TheModule;
    return 0;
}