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
        {
            SmallVector<Type*, 8> argTypes;
            Type *t_1xi32 = Type::getInt32Ty(TheContext);
            Type *t_2xi32 = FixedVectorType::get(t_1xi32, 2);
            Type *t_ptr_1xi32 = PointerType::get(t_1xi32, 0);
            Type *t_ptr_2xi32 = t_2xi32->getPointerTo(0);
            argTypes.push_back(t_ptr_1xi32);
            argTypes.push_back(t_ptr_2xi32);
            FunctionType *funcType = FunctionType::get(TheBuilder.getInt32Ty(), argTypes, false);
            Function *func = Function::Create(funcType, Function::ExternalLinkage, "test0", TheModule);

            BasicBlock *entryBB = BasicBlock::Create(TheContext, "entry", func);
            TheBuilder.SetInsertPoint(entryBB);
            TheBuilder.CreateGEP(TheBuilder.getInt32Ty(), func->getArg(0), TheBuilder.getInt32(0));
            TheBuilder.CreateGEP(TheBuilder.getInt32Ty(), func->getArg(1), TheBuilder.getInt32(0));
            TheBuilder.CreateGEP(TheBuilder.getInt32Ty(), func->getArg(1), TheBuilder.getInt32(1));
            TheBuilder.CreateRet(TheBuilder.getInt32(0));

            // optional set function argument name
            SmallVector<std::string, 8> argNames;
            argNames.push_back("a");
            argNames.push_back("b");
            unsigned i = 0;
            for (auto AI = func->arg_begin(), AE = func->arg_end(); AI != AE; AI++, i++) {
                AI->setName(argNames[i]);
            }
            verifyFunction(*func);
        }
        {
            Type *t_1xi32 = Type::getInt32Ty(TheContext);
            Type *t_ptr_1xi32 = PointerType::get(t_1xi32, 0);
            FunctionType *funcType = FunctionType::get(TheBuilder.getInt32Ty(), t_ptr_1xi32, false);
            Function *func = Function::Create(funcType, Function::ExternalLinkage, "test1", TheModule);

            BasicBlock *entryBB = BasicBlock::Create(TheContext, "entry", func);
            TheBuilder.SetInsertPoint(entryBB);
            auto *loadAddr = TheBuilder.CreateGEP(TheBuilder.getInt32Ty(), func->getArg(0), TheBuilder.getInt32(0));
            auto *loadValue = TheBuilder.CreateLoad(t_1xi32, loadAddr);
            TheBuilder.CreateStore(loadValue, loadAddr);
            TheBuilder.CreateRet(TheBuilder.getInt32(0));
            verifyFunction(*func);
        }
        {
            Type *t_1xi32 = Type::getInt32Ty(TheContext);
            Type *t_2xi32 = FixedVectorType::get(t_1xi32, 2);
            Type *t_ptr_2xi32 = t_2xi32->getPointerTo(0);
            FunctionType *funcType = FunctionType::get(TheBuilder.getVoidTy(), {t_ptr_2xi32, t_ptr_2xi32}, false);
            Function *func = Function::Create(funcType, Function::ExternalLinkage, "test2", TheModule);

            BasicBlock *entryBB = BasicBlock::Create(TheContext, "entry", func);
            TheBuilder.SetInsertPoint(entryBB);
            auto *loadAddr = TheBuilder.CreateGEP(TheBuilder.getInt32Ty(), func->getArg(0), TheBuilder.getInt32(0));
            auto *loadValue = TheBuilder.CreateLoad(t_2xi32, loadAddr);
            auto *V0 = TheBuilder.CreateExtractElement(loadValue, TheBuilder.getInt64(0));
            auto *V1 = TheBuilder.CreateExtractElement(loadValue, TheBuilder.getInt64(1));

            SmallVector<Constant *, 2> undef;
            undef.push_back(TheBuilder.getInt32(12));
            undef.push_back(TheBuilder.getInt32(0));
            loadAddr = TheBuilder.CreateGEP(TheBuilder.getInt32Ty(), func->getArg(1), TheBuilder.getInt32(0),"st_addr");
            auto *newValue = TheBuilder.CreateLoad(t_2xi32, loadAddr);
            TheBuilder.CreateInsertElement(newValue, V1, TheBuilder.getInt64(0));
            TheBuilder.CreateInsertElement(newValue, V0, TheBuilder.getInt64(1));
            TheBuilder.CreateStore(newValue, loadAddr);
            verifyFunction(*func);
        }
    }

    TheModule->dump();
    delete TheModule;
    return 0;
}