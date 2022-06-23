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
        {
            std::vector<Type *> argTypes;
            argTypes.push_back(Type::getInt32Ty(TheContext));
            argTypes.push_back(Type::getInt32Ty(TheContext));
            FunctionType *funcType = FunctionType::get(TheBuilder.getInt32Ty(), argTypes, false);
            Function *barFunc = Function::Create(funcType, Function::ExternalLinkage, "bar", TheModule);

            BasicBlock *entryBB = BasicBlock::Create(TheContext, "entry", barFunc);
            TheBuilder.SetInsertPoint(entryBB);
            TheBuilder.CreateRet(TheBuilder.getInt32(0));

            // optional set function argument name
            std::vector<std::string> funcArgs;
            funcArgs.push_back("a");
            funcArgs.push_back("b");
            unsigned i = 0;
            for (auto AI = barFunc->arg_begin(), AE = barFunc->arg_end(); AI != AE; AI++, i++) {
                AI->setName(funcArgs[i]);
            }
            verifyFunction(*barFunc);
        }
        {
            std::vector<Type *> argTypes;
            argTypes.push_back(Type::getInt32Ty(TheContext));
            argTypes.push_back(Type::getInt32Ty(TheContext));
            FunctionType *funcType = FunctionType::get(TheBuilder.getInt32Ty(), argTypes, false);
            Function *func = Function::Create(funcType, Function::ExternalLinkage, "curry", TheModule);

            BasicBlock *entryBB = BasicBlock::Create(TheContext, "entry", func);
            TheBuilder.SetInsertPoint(entryBB);
            Value *arg0 = func->arg_begin();
            Value *arg1 = func->getArg(1);
            TheBuilder.CreateRet(TheBuilder.CreateMul(arg0, arg1));
        }
        {
            std::vector<Type *> argTypes;
            argTypes.push_back(Type::getInt32Ty(TheContext));
            argTypes.push_back(Type::getInt32Ty(TheContext));
            FunctionType *funcType = FunctionType::get(TheBuilder.getInt32Ty(), argTypes, false);
            Function *func = Function::Create(funcType, Function::ExternalLinkage, "phi", TheModule);

            BasicBlock *entryBB = BasicBlock::Create(TheContext, "entry", func);
            BasicBlock *equalBB = BasicBlock::Create(TheContext, "==", func);
            BasicBlock *noneqBB = BasicBlock::Create(TheContext, "!=", func);
            BasicBlock *mergeBB = BasicBlock::Create(TheContext, "reconv", func);
            TheBuilder.SetInsertPoint(entryBB);
            Value *arg0 = func->arg_begin();
            Value *arg1 = func->getArg(1);
            Value *cond = TheBuilder.CreateICmpEQ(arg0, arg1);
            TheBuilder.CreateCondBr(cond, equalBB, noneqBB);

            TheBuilder.SetInsertPoint(equalBB);
            TheBuilder.CreateBr(mergeBB);
            TheBuilder.SetInsertPoint(noneqBB);
            TheBuilder.CreateBr(mergeBB);

            TheBuilder.SetInsertPoint(mergeBB);
            PHINode *phi = TheBuilder.CreatePHI(func->getReturnType(), 2);
            phi->addIncoming(TheBuilder.getInt32(1), equalBB);
            phi->addIncoming(TheBuilder.getInt32(0), noneqBB);
            TheBuilder.CreateRet(phi);
        }
    }

    TheModule->dump();
    delete TheModule;
    return 0;
}